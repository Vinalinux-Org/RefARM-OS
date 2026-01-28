/* ============================================================
 * timer.c
 * ------------------------------------------------------------
 * DMTimer2 driver implementation (test driver for IRQ)
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#include <stddef.h>
#include "timer.h"
#include "irq.h"
#include "intc.h"
#include "mmio.h"
#include "uart.h"

/* ============================================================
 * DMTimer2 Register Offsets
 * ============================================================ */

#define TIDR            0x00
#define TIOCP_CFG       0x10
#define IRQ_EOI         0x20
#define IRQSTATUS_RAW   0x24
#define IRQSTATUS       0x28
#define IRQENABLE_SET   0x2C
#define IRQENABLE_CLR   0x30
#define IRQWAKEEN       0x34
#define TCLR            0x38
#define TCRR            0x3C
#define TLDR            0x40
#define TTGR            0x44
#define TWPS            0x48
#define TMAR            0x4C
#define TCAR1           0x50
#define TSICR           0x54
#define TCAR2           0x58

/* TCLR bits */
#define TCLR_ST         (1 << 0)    /* Start/stop timer */
#define TCLR_AR         (1 << 1)    /* Auto-reload */

/* IRQSTATUS bits */
#define OVF_IT_FLAG     (1 << 1)    /* Overflow interrupt flag */

/* ============================================================
 * Timer State
 * ============================================================ */

static volatile uint32_t timer_ticks = 0;

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
    mmio_write32(DMTIMER2_BASE + IRQSTATUS, OVF_IT_FLAG);
    
    /* Update tick count */
    timer_ticks++;
    
    /* Log every 100 ticks (for testing) */
    if (timer_ticks % 100 == 0) {
        uart_printf("Timer tick: %u\n", timer_ticks);
    }
}

/* ============================================================
 * Timer Driver Implementation
 * ============================================================ */

void timer_init(void)
{
    uart_printf("Initializing Timer2...\n");
    
    /* Step 1: Stop timer */
    mmio_write32(DMTIMER2_BASE + TCLR, 0);
    
    /*
     * Step 2: Configure load value
     * Timer counts up from TLDR to 0xFFFFFFFF, then overflows
     * 
     * For fast overflow (testing):
     * TLDR = 0xFFFF0000 → overflow after 65536 counts
     * 
     * At 24MHz clock / 1 prescaler:
     * ~2.7ms per overflow
     * ~370 interrupts per second
     */
    mmio_write32(DMTIMER2_BASE + TLDR, 0xFFFF0000);
    mmio_write32(DMTIMER2_BASE + TCRR, 0xFFFF0000);  /* Set current counter */
    
    /* Step 3: Enable overflow interrupt at peripheral */
    mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, OVF_IT_FLAG);
    
    /* Step 4: Register IRQ handler with kernel */
    int ret = irq_register_handler(TIMER2_IRQ, timer_irq_handler, NULL);
    if (ret != 0) {
        uart_printf("ERROR: Failed to register Timer2 IRQ handler\n");
        return;
    }
    
    /* Step 5: Enable IRQ at INTC */
    intc_enable_irq(TIMER2_IRQ);
    
    /* Step 6: Start timer (auto-reload + enable) */
    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR | TCLR_ST);
    
    uart_printf("Timer2 initialized (IRQ %d)\n", TIMER2_IRQ);
    uart_printf("Timer will generate ~370 interrupts/second\n");
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}