/* ============================================================
 * uart.c
 * Simple UART0 Driver for AM335x
 * ============================================================ */

#include "am335x.h"
#include "boot.h"



/* Helper: Write 32-bit register */
/* writel is defined in am335x.h */

#define UART_FCLK_HZ    48000000    /* 48MHz */
#define UART_BAUDRATE   115200

#define UART_LCR_8N1    0x03
#define UART_LCR_BAUD_SETUP 0x83  /* Divisor Latch Enable + 8N1 */
#define UART_MDR1_16X   0x00
#define UART_DISABLE    0x07

/* Calculate divisor for 16x mode */
/* Divisor = FCLK / (16 * Baud) */
/* 48000000 / (16 * 115200) = 26.04 -> 26 */

void uart_flush(void)
{
    /* Wait for TX FIFO and Shift Register to be empty */
    while ((readl(UART0_LSR) & UART_LSR_TEMT) == 0);
}

void uart_init(void)
{
    uint32_t divisor;
    uint32_t i;
    
    /* IMPORTANT: According to BootROM doc Table 26-6, UART_CLK = 48MHz
     * from PER_CLKOUTM2. This should remain constant even after DDR PLL config.
     * However, let's be safe and use a more robust initialization sequence.
     */
    
    /* 1. Hard reset UART by disabling it completely */
    writel(UART_DISABLE, UART0_MDR1);
    delay(10000);
    
    /* 2. Clear FIFOs and drain any garbage */
    writel(0x07, UART0_FCR);  /* Enable + Reset TX/RX FIFOs */
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
     * Divisor = 48,000,000 / (16 * 115200) = 26.0416 ≈ 26
     * Actual baud = 48,000,000 / (16 * 26) = 115,384.6 (0.16% error - acceptable)
     */
    divisor = 26;  /* Fixed value for 48MHz clock */
    
    /* Enable divisor latch access */
    writel(UART_LCR_BAUD_SETUP, UART0_LCR);
    
    /* Set divisor */
    writel(divisor & 0xFF, UART0_DLL);
    writel((divisor >> 8) & 0xFF, UART0_DLH);
    
    /* 4. Configure line: 8N1 (8 data bits, no parity, 1 stop) */
    writel(UART_LCR_8N1, UART0_LCR);
    
    /* 5. Configure FIFO (enable, clear, trigger level) */
    writel(0x07, UART0_FCR);  /* Enable + Reset TX/RX, trigger level 1 */
    delay(1000);
    
    /* 6. Enable UART in 16x mode */
    writel(UART_16X_MODE, UART0_MDR1);
    
    /* 7. Wait for UART to stabilize */
    delay(10000);
    
    /* 8. Final flush */
    while ((readl(UART0_LSR) & UART_LSR_TEMT) == 0);
}

/* Put single character */
void uart_putc(char c)
{
    /* Handle newline: send CR BEFORE LF */
    if (c == '\n') {
        uart_putc('\r');
    }
    
    /* Wait for THRE (TX FIFO Empty) BEFORE writing */
    while ((readl(UART0_LSR) & UART_LSR_TXFIFOE) == 0);
    
    /* Write character */
    writeb(c, UART0_THR);
    
    /* Delay to ensure Write propagates to LSR status update */
    { volatile int d; for(d=0; d<100; d++); }

    /* Wait for THRE (TX FIFO Empty) AFTER writing */
    while ((readl(UART0_LSR) & UART_LSR_TXFIFOE) == 0);
}

/* Put string */
void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

/* Print hex value (Robust) */
void uart_print_hex(uint32_t val)
{
    int i;
    static const char hex_digits[] = "0123456789ABCDEF"; 
    
    uart_puts("0x");
    
    for (i = 28; i >= 0; i -= 4) {
        int nibble = (val >> i) & 0xF;
        if (nibble < 0 || nibble > 15) {
            uart_putc('?');
        } else {
            uart_putc(hex_digits[nibble]);
        }
    }
}