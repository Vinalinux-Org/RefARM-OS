/* ============================================================
 * uart.c
 * UART0 Driver for AM335x (BeagleBone Black)
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* UART configuration constants */
#define UART_FCLK_HZ    48000000    /* 48MHz (from PER PLL) */
#define UART_BAUDRATE   115200

#define UART_LCR_8N1    0x03        /* 8 data bits, no parity, 1 stop */
#define UART_LCR_BAUD_SETUP 0x83    /* Divisor Latch Enable + 8N1 */
#define UART_MDR1_16X   0x00        /* 16x oversampling mode */
#define UART_DISABLE    0x07        /* Disable UART mode */

/* Baud rate divisor calculation:
 * Divisor = FCLK / (16 * Baud) = 48,000,000 / (16 * 115200) ≈ 26 */

void uart_flush(void)
{
    /* Wait for TX FIFO and shift register to be empty */
    while ((readl(UART0_LSR) & UART_LSR_TEMT) == 0);
}

void uart_init(void)
{
    uint32_t divisor;
    uint32_t i;
    
    /* UART clock is 48MHz from PER PLL (configured by ROM) */
    
    /* 1. Hard reset UART by disabling it */
    writel(UART_DISABLE, UART0_MDR1);
    delay(10000);
    
    /* 2. Clear FIFOs and drain any garbage data */
    writel(0x07, UART0_FCR);  /* Enable + reset TX/RX FIFOs */
    delay(1000);
    
    /* Drain RX FIFO completely */
    for (i = 0; i < 16; i++) {
        if (readl(UART0_LSR) & UART_LSR_RXFIFOE) {
            (void)readl(UART0_RHR);
        } else {
            break;
        }
    }
    
    /* 3. Configure baud rate: 115200 @ 48MHz
     * Divisor = 48,000,000 / (16 * 115200) ≈ 26
     * Actual baud rate: 115,384.6 bps (0.16% error, acceptable) */
    divisor = 26;
    
    /* Enable divisor latch access */
    writel(UART_LCR_BAUD_SETUP, UART0_LCR);
    
    /* Set divisor (16-bit value) */
    writel(divisor & 0xFF, UART0_DLL);
    writel((divisor >> 8) & 0xFF, UART0_DLH);
    
    /* 4. Configure line format: 8N1 */
    writel(UART_LCR_8N1, UART0_LCR);
    
    /* 5. Configure FIFO (enable, clear, trigger level) */
    writel(0x07, UART0_FCR);
    delay(1000);
    
    /* 6. Enable UART in 16x oversampling mode */
    writel(UART_16X_MODE, UART0_MDR1);
    
    /* 7. Wait for UART to stabilize */
    delay(10000);
    
    /* 8. Final flush */
    while ((readl(UART0_LSR) & UART_LSR_TEMT) == 0);
}

/* Send single character */
void uart_putc(char c)
{
    /* Handle newline: send CR before LF */
    if (c == '\n') {
        uart_putc('\r');
    }
    
    /* Wait for TX FIFO empty before writing */
    while ((readl(UART0_LSR) & UART_LSR_TXFIFOE) == 0);
    
    /* Write character to transmit register */
    writeb(c, UART0_THR);
    
    /* Small delay for status update */
    { volatile int d; for(d=0; d<100; d++); }

    /* Wait for TX FIFO empty after writing */
    while ((readl(UART0_LSR) & UART_LSR_TXFIFOE) == 0);
}

/* Send string */
void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

/* Print hexadecimal value */
void uart_print_hex(uint32_t val)
{
    int i;
    static const char hex_digits[] = "0123456789ABCDEF"; 
    
    uart_puts("0x");
    
    /* Print 8 hexadecimal digits (32 bits) */
    for (i = 28; i >= 0; i -= 4) {
        int nibble = (val >> i) & 0xF;
        uart_putc(hex_digits[nibble]);
    }
}