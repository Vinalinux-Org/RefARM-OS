/* ============================================================
 * uart.c
 * ------------------------------------------------------------
 * UART driver implementation
 * Target: AM335x UART0
 * ============================================================
 */

#include "uart.h"
#include "mmio.h"

/* ============================================================
 * Hardware definitions
 * ============================================================
 */
#define UART0_BASE      0x44E09000

/* UART registers (offsets from UART0_BASE) */
#define UART_THR        0x00    /* Transmit Holding Register */
#define UART_LSR        0x14    /* Line Status Register */

/* Line Status Register bits */
#define UART_LSR_THRE   (1 << 5) /* Transmit Holding Register Empty */

/* ============================================================
 * Basic UART operations
 * ============================================================
 */

void uart_init(void)
{
    /* Placeholder: bootloader already configured UART
     * Future: add full UART initialization here
     * - Set baud rate (via DLL/DLH registers)
     * - Configure frame format (LCR register)
     * - Enable FIFOs (FCR register)
     * - Configure interrupts (IER register)
     */
}

void uart_putc(char c)
{
    /* Wait for TX FIFO to be ready */
    while (!(mmio_read32(UART0_BASE + UART_LSR) & UART_LSR_THRE));
    
    /* Write character to transmit holding register */
    mmio_write32(UART0_BASE + UART_THR, c);
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');  /* LF -> CRLF conversion */
        uart_putc(*s++);
    }
}

/* ============================================================
 * Helper functions for printf
 * ============================================================
 */

/**
 * Print unsigned integer in given base
 * Used internally by uart_printf
 */
static void print_uint(uint32_t num, int base, int uppercase)
{
    char buf[32];
    int i = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    /* Handle zero specially */
    if (num == 0) {
        uart_putc('0');
        return;
    }
    
    /* Convert to string (reverse order) */
    while (num > 0) {
        buf[i++] = digits[num % base];
        num /= base;
    }
    
    /* Print in correct order */
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

/**
 * Print signed integer
 */
static void print_int(int32_t num)
{
    if (num < 0) {
        uart_putc('-');
        num = -num;
    }
    print_uint((uint32_t)num, 10, 0);
}

/**
 * Print hex with padding
 * Example: print_hex(0x42, 8) -> "00000042"
 */
static void print_hex(uint32_t num, int width, int uppercase)
{
    char buf[8];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i;
    
    /* Convert to hex string */
    for (i = 7; i >= 0; i--) {
        buf[i] = digits[num & 0xF];
        num >>= 4;
    }
    
    /* Print with width */
    if (width > 8)
        width = 8;
    if (width < 1)
        width = 1;
    
    for (i = 8 - width; i < 8; i++) {
        uart_putc(buf[i]);
    }
}

/* ============================================================
 * Formatted print implementation
 * ============================================================
 */

void uart_printf(const char *fmt, ...)
{
    va_list args;
    const char *p;
    int width;
    
    va_start(args, fmt);
    
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            /* Regular character */
            if (*p == '\n')
                uart_putc('\r');
            uart_putc(*p);
            continue;
        }
        
        /* Format specifier */
        p++;
        
        /* Parse width for hex (e.g., %08x) */
        width = 0;
        if (*p == '0') {
            p++;
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }
        }
        
        switch (*p) {
        case 'd':  /* Signed decimal */
        case 'i':
            print_int(va_arg(args, int));
            break;
            
        case 'u':  /* Unsigned decimal */
            print_uint(va_arg(args, uint32_t), 10, 0);
            break;
            
        case 'x':  /* Hex lowercase */
            if (width > 0)
                print_hex(va_arg(args, uint32_t), width, 0);
            else
                print_uint(va_arg(args, uint32_t), 16, 0);
            break;
            
        case 'X':  /* Hex uppercase */
            if (width > 0)
                print_hex(va_arg(args, uint32_t), width, 1);
            else
                print_uint(va_arg(args, uint32_t), 16, 1);
            break;
            
        case 's':  /* String */
            {
                const char *s = va_arg(args, const char *);
                if (s)
                    uart_puts(s);
                else
                    uart_puts("(null)");
            }
            break;
            
        case 'c':  /* Character */
            uart_putc((char)va_arg(args, int));
            break;
            
        case '%':  /* Literal % */
            uart_putc('%');
            break;
            
        default:
            /* Unknown format - print as-is */
            uart_putc('%');
            uart_putc(*p);
            break;
        }
    }
    
    va_end(args);
}