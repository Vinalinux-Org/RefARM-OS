/* ============================================================
 * uart_with_clock.c - UART with Clock Enable (FIXED)
 * ------------------------------------------------------------
 * Purpose: Enable UART0 module clock before RX config
 * Target: AM335x UART0
 * ============================================================
 */

#include <stddef.h>
#include "uart.h"
#include "mmio.h"
#include "cpu.h"
#include "irq.h"
#include "intc.h"
#include <stdarg.h>

/* ============================================================
 * Hardware Definitions
 * ============================================================
 */

#define UART0_BASE      0x44E09000
#define UART0_IRQ       72

/* Clock Module */
#define CM_PER_BASE             0x44E00000
#define CM_PER_UART0_CLKCTRL    (CM_PER_BASE + 0x6C)

/* UART Register Offsets */
#define UART_RHR        0x00
#define UART_THR        0x00
#define UART_IER        0x04
#define UART_IIR        0x08
#define UART_FCR        0x08
#define UART_EFR        0x08
#define UART_LCR        0x0C
#define UART_LSR        0x14
#define UART_SCR        0x40

/* IER bits */
#define IER_RHR_IT      (1 << 0)

/* IIR bits */
#define IIR_IT_PENDING  (1 << 0)
#define IIR_IT_TYPE_MASK 0x3E
#define IIR_RHR_IT      0x04
#define IIR_CHAR_TIMEOUT 0x0C

/* FCR bits */
#define FCR_FIFO_EN     (1 << 0)
#define FCR_RX_CLR      (1 << 1)
#define FCR_TX_CLR      (1 << 2)

/* LSR bits */
#define LSR_DR          (1 << 0)
#define LSR_OE          (1 << 1)
#define LSR_PE          (1 << 2)
#define LSR_FE          (1 << 3)
#define LSR_BI          (1 << 4)
#define LSR_THRE        (1 << 5)
#define LSR_ERROR_MASK  (LSR_OE | LSR_PE | LSR_FE | LSR_BI)

/* ============================================================
 * Ring Buffer
 * ============================================================
 */

struct uart_rx_buffer {
    uint8_t data[UART_RX_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t overflow;
};

static struct uart_rx_buffer rx_buffer;
static volatile uint32_t uart_irq_fire_count = 0;

/* ============================================================
 * RX Interrupt Handler
 * ============================================================
 */

static void uart_rx_irq_handler(void *data)
{
    uart_irq_fire_count++;
    
    uart_printf("[UART_IRQ] Entry #%u\n", uart_irq_fire_count);
    
    uint32_t iir, lsr;
    uint8_t ch;
    
    /* Read IIR */
    iir = mmio_read32(UART0_BASE + UART_IIR);
    uart_printf("[UART_IRQ] IIR = 0x%02x\n", iir);
    
    /* Check if interrupt pending */
    if (iir & IIR_IT_PENDING) {
        uart_printf("[UART_IRQ] No interrupt pending (spurious?)\n");
        return;
    }
    
    /* Extract interrupt type */
    uint32_t it_type = (iir & IIR_IT_TYPE_MASK) >> 1;
    uart_printf("[UART_IRQ] IT_TYPE = 0x%02x ", it_type);
    
    if (it_type == IIR_RHR_IT) {
        uart_printf("(RHR data available)\n");
    } else if (it_type == IIR_CHAR_TIMEOUT) {
        uart_printf("(char timeout)\n");
    } else if (it_type == 0x06) {
        uart_printf("(LINE STATUS - checking errors)\n");
    } else {
        uart_printf("(other: 0x%02x)\n", it_type);
    }
    
    /* Handle LINE STATUS interrupt (IT_TYPE = 0x06) */
    if (it_type == 0x06) {
        /* Read LSR to get error status */
        lsr = mmio_read32(UART0_BASE + UART_LSR);
        uart_printf("[UART_IRQ] LSR = 0x%02x\n", lsr);
        
        if (lsr & LSR_OE) uart_printf("[UART_IRQ]   OE (Overrun Error)\n");
        if (lsr & LSR_PE) uart_printf("[UART_IRQ]   PE (Parity Error)\n");
        if (lsr & LSR_FE) uart_printf("[UART_IRQ]   FE (Framing Error)\n");
        if (lsr & LSR_BI) uart_printf("[UART_IRQ]   BI (Break Interrupt)\n");
        
        /* Check if data available despite error */
        if (lsr & LSR_DR) {
            uart_printf("[UART_IRQ] Data available despite error - reading...\n");
            /* Fall through to read data */
        } else {
            uart_printf("[UART_IRQ] No data available\n");
            return;  /* Exit, error cleared by reading LSR */
        }
    }
    
    /* Handle RX interrupts */
    if ((it_type == IIR_RHR_IT) || (it_type == IIR_CHAR_TIMEOUT) || (it_type == 0x06)) {
        int char_count = 0;
        
        while (1) {
            lsr = mmio_read32(UART0_BASE + UART_LSR);
            
            /* Check for errors */
            if (lsr & LSR_ERROR_MASK) {
                uart_printf("[UART_IRQ] Error in LSR: 0x%02x\n", lsr);
                (void)mmio_read32(UART0_BASE + UART_RHR);
                continue;
            }
            
            /* Check if data ready */
            if (!(lsr & LSR_DR)) {
                uart_printf("[UART_IRQ] No more data (LSR.DR=0)\n");
                break;
            }
            
            /* Read character */
            ch = mmio_read32(UART0_BASE + UART_RHR) & 0xFF;
            char_count++;
            
            uart_printf("[UART_IRQ] Read char #%d: 0x%02x ('", char_count, ch);
            if (ch >= 32 && ch < 127) {
                uart_putc(ch);
            } else {
                uart_putc('?');
            }
            uart_printf("')\n");
            
            /* Store in ring buffer */
            uint32_t next_head = (rx_buffer.head + 1) % UART_RX_BUFFER_SIZE;
            
            if (next_head == rx_buffer.tail) {
                rx_buffer.overflow++;
                uart_printf("[UART_IRQ] Buffer overflow!\n");
                continue;
            }
            
            rx_buffer.data[rx_buffer.head] = ch;
            rx_buffer.head = next_head;
        }
        
        uart_printf("[UART_IRQ] Total chars read: %d\n", char_count);
    } else {
        uart_printf("[UART_IRQ] Non-RX interrupt type\n");
    }
    
    uart_printf("[UART_IRQ] Handler exit\n");
}

/* ============================================================
 * UART Initialization
 * ============================================================
 */

void uart_init(void)
{
    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    rx_buffer.overflow = 0;
}

void uart_enable_rx_interrupt(void)
{
    uart_printf("\n=== UART RX Interrupt Config (WITH CLOCK ENABLE) ===\n\n");
    
    /* ===== STEP 0: Enable UART0 Module Clock (CRITICAL!) ===== */
    uart_printf("[STEP 0] Enabling UART0 module clock...\n");
    
    uint32_t clkctrl = mmio_read32(CM_PER_UART0_CLKCTRL);
    uart_printf("  CM_PER_UART0_CLKCTRL (before) = 0x%08x\n", clkctrl);
    
    /* Enable module: MODULEMODE = 0x2 (ENABLE) */
    mmio_write32(CM_PER_UART0_CLKCTRL, 0x2);
    
    /* Wait for module to be fully functional */
    uart_printf("  Waiting for IDLEST = FUNC (functional)...\n");
    uint32_t timeout = 1000000;
    while (timeout-- > 0) {
        clkctrl = mmio_read32(CM_PER_UART0_CLKCTRL);
        /* IDLEST bits [17:16]: 0x0 = functional, 0x3 = disabled */
        if ((clkctrl & 0x30000) == 0) {
            break;
        }
    }
    
    if (timeout == 0) {
        uart_printf("  WARNING: Timeout waiting for UART0 clock!\n");
    } else {
        uart_printf("  SUCCESS: UART0 clock enabled\n");
    }
    
    clkctrl = mmio_read32(CM_PER_UART0_CLKCTRL);
    uart_printf("  CM_PER_UART0_CLKCTRL (after) = 0x%08x\n", clkctrl);
    uart_printf("    MODULEMODE (bits 1:0) = %u (2=enabled)\n", clkctrl & 0x3);
    uart_printf("    IDLEST (bits 17:16) = %u (0=functional)\n", (clkctrl >> 16) & 0x3);
    uart_printf("\n");
    
    /* ===== STEP 1: Simple Legacy Config (No ENHANCED mode) ===== */
    uart_printf("[STEP 1] Configuring UART (legacy mode)...\n");
    
    /* Save LCR */
    uint32_t lcr_save = mmio_read32(UART0_BASE + UART_LCR);
    uart_printf("  LCR (saved) = 0x%08x\n", lcr_save);
    
    /* Make sure in operational mode */
    mmio_write32(UART0_BASE + UART_LCR, lcr_save & 0x7F);
    uart_printf("  LCR = operational mode\n");
    
    /* Clear IER */
    mmio_write32(UART0_BASE + UART_IER, 0x00);
    uart_printf("  IER = 0x00 (cleared)\n");
    
    /* Configure FCR - simple mode */
    uart_printf("  Writing FCR = 0x07 (FIFO enable, 8-char trigger)...\n");
    mmio_write32(UART0_BASE + UART_FCR, 0x07);
    
    /* Small delay */
    for (volatile int i = 0; i < 10000; i++);
    
    /* Configure SCR for 1-char granularity */
    uart_printf("  Writing SCR = 0xC0 (1-char granularity)...\n");
    mmio_write32(UART0_BASE + UART_SCR, 0xC0);
    
    uint32_t scr = mmio_read32(UART0_BASE + UART_SCR);
    uart_printf("  SCR (readback) = 0x%08x\n", scr);
    uart_printf("    RX_TRIG_GRANU1 (bit 7) = %u\n", (scr >> 7) & 1);
    
    /* Restore LCR */
    mmio_write32(UART0_BASE + UART_LCR, lcr_save);
    
    /* Enable RHR interrupt */
    uart_printf("  Writing IER = 0x01 (RHR_IT)...\n");
    mmio_write32(UART0_BASE + UART_IER, IER_RHR_IT);
    
    uint32_t ier = mmio_read32(UART0_BASE + UART_IER);
    uart_printf("  IER (readback) = 0x%08x\n", ier);
    uart_printf("    RHR_IT (bit 0) = %u\n", ier & 1);
    
    /* Verify FIFO */
    uint32_t iir = mmio_read32(UART0_BASE + UART_IIR);
    uart_printf("  IIR = 0x%08x\n", iir);
    uart_printf("    FCR_MIRROR (bit 6) = %u (1=FIFO enabled)\n", (iir >> 6) & 1);
    
    /* Check LSR */
    uint32_t lsr = mmio_read32(UART0_BASE + UART_LSR);
    uart_printf("  LSR = 0x%08x\n", lsr);
    uart_printf("    DR (bit 0) = %u\n", lsr & 1);
    uart_printf("\n");
    
    /* ===== STEP 2: Register IRQ Handler ===== */
    uart_printf("[STEP 2] Registering IRQ handler...\n");
    int ret = irq_register_handler(UART0_IRQ, uart_rx_irq_handler, NULL);
    if (ret != 0) {
        uart_printf("  ERROR: Handler registration failed!\n");
        return;
    }
    uart_printf("  Handler registered OK\n\n");
    
    /* ===== STEP 3: Enable IRQ in INTC ===== */
    uart_printf("[STEP 3] Enabling IRQ 72 in INTC...\n");
    intc_enable_irq(UART0_IRQ);
    uart_printf("  IRQ 72 enabled in INTC\n\n");
    
    uart_printf("=== UART RX Interrupt Config COMPLETE ===\n\n");
}

/* ============================================================
 * TX Functions
 * ============================================================
 */

void uart_putc(char c)
{
    while (!(mmio_read32(UART0_BASE + UART_LSR) & LSR_THRE));
    mmio_write32(UART0_BASE + UART_THR, c);
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/* ============================================================
 * RX Functions
 * ============================================================
 */

int uart_getc(void)
{
    uint32_t flags = irq_save();
    
    if (rx_buffer.head == rx_buffer.tail) {
        irq_restore(flags);
        return -1;
    }
    
    uint8_t ch = rx_buffer.data[rx_buffer.tail];
    rx_buffer.tail = (rx_buffer.tail + 1) % UART_RX_BUFFER_SIZE;
    
    irq_restore(flags);
    return (int)ch;
}

int uart_rx_available(void)
{
    uint32_t head = rx_buffer.head;
    uint32_t tail = rx_buffer.tail;
    
    if (head >= tail) {
        return head - tail;
    } else {
        return UART_RX_BUFFER_SIZE - tail + head;
    }
}

void uart_rx_clear(void)
{
    uint32_t flags = irq_save();
    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    irq_restore(flags);
}

uint32_t uart_get_irq_fire_count(void)
{
    return uart_irq_fire_count;
}

/* ============================================================
 * Printf
 * ============================================================
 */

static void print_uint(uint32_t num, int base)
{
    char buf[32];
    int i = 0;
    const char *digits = "0123456789abcdef";
    
    if (num == 0) {
        uart_putc('0');
        return;
    }
    
    while (num > 0) {
        buf[i++] = digits[num % base];
        num /= base;
    }
    
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    const char *p;
    
    va_start(args, fmt);
    
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            if (*p == '\n')
                uart_putc('\r');
            uart_putc(*p);
            continue;
        }
        
        p++;
        
        int width = 0;
        if (*p == '0') {
            p++;
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }
        }
        
        switch (*p) {
        case 'd':
        case 'u':
            print_uint(va_arg(args, uint32_t), 10);
            break;
        case 'x':
        case 'X':
            print_uint(va_arg(args, uint32_t), 16);
            break;
        case 's':
            {
                const char *s = va_arg(args, const char *);
                if (s) uart_puts(s);
                else uart_puts("(null)");
            }
            break;
        case 'c':
            uart_putc((char)va_arg(args, int));
            break;
        case '%':
            uart_putc('%');
            break;
        default:
            uart_putc('%');
            uart_putc(*p);
            break;
        }
    }
    
    va_end(args);
}