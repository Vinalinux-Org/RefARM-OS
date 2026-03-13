/* ============================================================
 * timer.c
 * ------------------------------------------------------------
 * DMTimer2 driver implementation
 * ============================================================ */

#include "types.h"
#include "timer.h"
#include "irq.h"
#include "scheduler.h"
#include "intc.h"
#include "mmio.h"
#include "uart.h"
#include "trace.h"

/* ============================================================
 * Clock Management Registers
 * ============================================================ */

#define CM_PER_BASE             0x44E00000
#define CM_PER_L4LS_CLKSTCTRL   (CM_PER_BASE + 0x00)
#define CM_PER_TIMER2_CLKCTRL   (CM_PER_BASE + 0x80)

/* CM_PER_L4LS_CLKSTCTRL bits */
#define CLKTRCTRL_MASK          0x3
#define CLKTRCTRL_SW_WKUP       0x2      /* Force wakeup */

/* CM_PER_TIMER2_CLKCTRL bits */
#define MODULEMODE_MASK         0x3
#define MODULEMODE_ENABLE       0x2      /* Module explicitly enabled */
#define IDLEST_MASK             0x3
#define IDLEST_SHIFT            16
#define IDLEST_FUNC             0x0      /* Fully functional */

/* ============================================================
 * DMTimer2 Register Offsets
 * ============================================================ */

#define TIDR            0x00    /* Identification register */
#define TIOCP_CFG       0x10    /* OCP interface configuration */
#define TISTAT          0x14    /* Status (reset) */
#define IRQSTATUS_RAW   0x24    /* IRQ raw status */
#define IRQSTATUS       0x28    /* IRQ status (after mask) */
#define IRQENABLE_SET   0x2C    /* IRQ enable set */
#define IRQENABLE_CLR   0x30    /* IRQ enable clear */
#define TCLR            0x38    /* Control register */
#define TCRR            0x3C    /* Counter register */
#define TLDR            0x40    /* Load register */
#define TTGR            0x44    /* Trigger register */
#define TWPS            0x48    /* Write-posted pending */
#define TMAR            0x4C    /* Match register */
#define TCAR1           0x50    /* Capture register 1 */
#define TSICR           0x54    /* Synchronous interface control */
#define TCAR2           0x58    /* Capture register 2 */

/* TIOCP_CFG bits */
#define TIOCP_SOFTRESET         (1 << 0)

/* TISTAT bits */
#define TISTAT_RESETDONE        (1 << 0)

/* TSICR bits */
#define TSICR_POSTED            (1 << 2)

/* TWPS bits */
#define TWPS_W_PEND_TCLR        (1 << 0)
#define TWPS_W_PEND_TCRR        (1 << 1)
#define TWPS_W_PEND_TLDR        (1 << 2)

/* TCLR bits */
#define TCLR_ST                 (1 << 0)    /* Start/stop timer */
#define TCLR_AR                 (1 << 1)    /* Auto-reload */

/* IRQSTATUS bits */
#define IRQ_MAT_IT_FLAG         (1 << 0)    /* Match interrupt */
#define IRQ_OVF_IT_FLAG         (1 << 1)    /* Overflow interrupt */
#define IRQ_TCAR_IT_FLAG        (1 << 2)    /* Capture interrupt */

/* ============================================================
 * Timer State
 * ============================================================ */

static volatile uint32_t timer_ticks = 0;

/* ============================================================
 * Timer Clock Configuration
 * ============================================================ */

/**
 * Enable DMTimer2 module clock
 * 
 * Clock initialization steps:
 * 1. Wake up L4LS clock domain
 * 2. Enable module clock
 * 3. Wait until module is fully functional
 * 
 * CRITICAL: Must be called before any timer register access
 */
static void timer2_clock_enable(void)
{
    uint32_t val;
    
    uart_printf("[TIMER] Enabling Timer2 clock...\n");
    
    /* Step 1: Ensure L4LS clock domain is awake */
    val = mmio_read32(CM_PER_L4LS_CLKSTCTRL);
    uart_printf("  L4LS_CLKSTCTRL = 0x%08x\n", val);
    
    if ((val & CLKTRCTRL_MASK) != CLKTRCTRL_SW_WKUP) {
        uart_printf("  L4LS domain not awake, forcing wakeup...\n");
        mmio_write32(CM_PER_L4LS_CLKSTCTRL, CLKTRCTRL_SW_WKUP);
        
        /* Wait for domain transition */
        uint32_t timeout = 10000;
        while (((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & CLKTRCTRL_MASK) != CLKTRCTRL_SW_WKUP) && timeout--) {
            /* Busy wait */
        }
        
        if (!timeout) {
            uart_printf("  [ERROR] L4LS wakeup timeout!\n");
            while (1);  /* Halt */
        }
        
        uart_printf("  L4LS domain now awake\n");
    } else {
        uart_printf("  L4LS domain already awake\n");
    }
    
    /* Step 2: Enable Timer2 module clock */
    val = mmio_read32(CM_PER_TIMER2_CLKCTRL);
    uart_printf("  TIMER2_CLKCTRL (before) = 0x%08x\n", val);
    
    mmio_write32(CM_PER_TIMER2_CLKCTRL, MODULEMODE_ENABLE);
    
    /* Step 3: Wait for module to become fully functional */
    uart_printf("  Waiting for IDLEST = FUNCTIONAL...\n");
    
    uint32_t timeout = 100000;
    while (timeout--) {
        val = mmio_read32(CM_PER_TIMER2_CLKCTRL);
        
        uint32_t idlest = (val >> IDLEST_SHIFT) & IDLEST_MASK;
        uint32_t modulemode = val & MODULEMODE_MASK;
        
        if (idlest == IDLEST_FUNC && modulemode == MODULEMODE_ENABLE) {
            uart_printf("  Timer2 clock fully functional\n");
            uart_printf("  TIMER2_CLKCTRL (after) = 0x%08x\n", val);
            return;
        }
    }
    
    /* Timeout - critical error */
    uart_printf("  [ERROR] Timer2 clock enable timeout!\n");
    uart_printf("  TIMER2_CLKCTRL = 0x%08x\n", mmio_read32(CM_PER_TIMER2_CLKCTRL));
    while (1);  /* Halt */
}

/* ============================================================
 * Timer IRQ Handler
 * ============================================================ */

static void timer_irq_handler(void *data)
{
    /*
     * Timer IRQ Handler
     * 
     * CONTRACT:
     * - Must clear interrupt status at peripheral
     * - Must be fast (no blocking operations)
     * - INTC EOI will be called by irq_dispatch()
     */
    
    /* Clear interrupt status (write 1 to clear) */
    mmio_write32(DMTIMER2_BASE + IRQSTATUS, IRQ_OVF_IT_FLAG);
    
    /* Update tick count */
    timer_ticks++;
    
    /* 
     * Log heartbeat (optional, can be very noisy) 
     * TRACE gets compiled out if disabled
     */
    if (timer_ticks % 100 == 0) {
        // TRACE_SCHED("Tick %u", timer_ticks);
    }
    
    /* Call scheduler prompt (State Machine: Set Flag) */
    scheduler_tick();
}

/* ============================================================
 * Timer Driver Implementation
 * ============================================================ */

void timer_init(void)
{
    uart_printf("[TIMER] Initializing DMTimer2...\n");
    
    /* Step 0: Enable clock (CRITICAL - must be first) */
    timer2_clock_enable();
    
    /* Step 1: Soft reset timer */
    uart_printf("[TIMER] Performing soft reset...\n");
    mmio_write32(DMTIMER2_BASE + TIOCP_CFG, TIOCP_SOFTRESET);
    
    /* Wait for reset done */
    uint32_t timeout = 10000;
    while (!(mmio_read32(DMTIMER2_BASE + TISTAT) & TISTAT_RESETDONE) && timeout--) {
        /* Busy wait */
    }
    
    if (!timeout) {
        uart_printf("  [ERROR] Timer soft reset timeout!\n");
        while (1);
    }
    uart_printf("  Soft reset complete\n");
    
    /* Step 2: Configure posted mode for better performance */
    uart_printf("[TIMER] Configuring posted mode...\n");
    mmio_write32(DMTIMER2_BASE + TSICR, TSICR_POSTED);
    uart_printf("  Posted mode enabled\n");
    
    /* Step 3: Stop timer */
    uart_printf("[TIMER] Stopping timer...\n");
    mmio_write32(DMTIMER2_BASE + TCLR, 0);
    
    /* Poll write-posted status */
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--);
    
    /* Step 4: Clear interrupts */
    uart_printf("[TIMER] Clearing interrupts...\n");
    mmio_write32(DMTIMER2_BASE + IRQSTATUS, 0x7);  /* Clear all */
    
    /* Step 5: Configure reload value for 10ms @ 24MHz
     * Count = 24,000,000 * 0.01 = 240,000
     * Reload = 0xFFFFFFFF - 240,000 + 1 */
    uint32_t freq = 24000000;
    uint32_t period_ms = 10;
    uint32_t count = (freq / 1000) * period_ms;
    uint32_t reload = 0xFFFFFFFF - count + 1;
    
    uart_printf("[TIMER] Configuring %ums period...\n", period_ms);
    uart_printf("  FCLK = %u Hz\n", freq);
    uart_printf("  Count = %u (0x%x)\n", count, count);
    uart_printf("  Reload = 0x%08x\n", reload);
    
    /* Set load register */
    mmio_write32(DMTIMER2_BASE + TLDR, reload);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TLDR) && timeout--);
    
    /* Set counter register */
    mmio_write32(DMTIMER2_BASE + TCRR, reload);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCRR) && timeout--);
    
    /* Step 6: Enable overflow interrupt */
    uart_printf("[TIMER] Enabling overflow interrupt...\n");
    mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, IRQ_OVF_IT_FLAG);
    
    /* Step 7: Register IRQ handler */
    int ret = irq_register_handler(TIMER2_IRQ, timer_irq_handler, NULL);
    if (ret != 0) {
        uart_printf("  [ERROR] Failed to register IRQ handler\n");
        return;
    }
    
    /* Step 8: Enable IRQ at interrupt controller */
    intc_enable_irq(TIMER2_IRQ);
    
    /* Step 9: Configure auto-reload mode (but don't start yet) */
    uart_printf("[TIMER] Configuring auto-reload mode...\n");
    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR);  /* AR=1, ST=0 */
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--);
    
    uart_printf("[TIMER] Initialization complete (timer stopped)\n");
    uart_printf("  TIOCP_CFG = 0x%08x\n", mmio_read32(DMTIMER2_BASE + TIOCP_CFG));
    uart_printf("  TSICR = 0x%08x\n", mmio_read32(DMTIMER2_BASE + TSICR));
    uart_printf("  TCLR = 0x%08x\n", mmio_read32(DMTIMER2_BASE + TCLR));
    uart_printf("  TLDR = 0x%08x\n", mmio_read32(DMTIMER2_BASE + TLDR));
    uart_printf("  TCRR = 0x%08x\n", mmio_read32(DMTIMER2_BASE + TCRR));
    
    /* Step 10: Start timer */
    uint32_t tclr = mmio_read32(DMTIMER2_BASE + TCLR);
    tclr |= TCLR_ST;  /* Set START bit */
    mmio_write32(DMTIMER2_BASE + TCLR, tclr);
    
    uart_printf("[TIMER] Timer started!\n");
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}