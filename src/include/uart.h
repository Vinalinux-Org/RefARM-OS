/* ============================================================
 * uart.h
 * ------------------------------------------------------------
 * UART driver interface
 * Target: AM335x UART0
 * ============================================================
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdarg.h>

/* ============================================================
 * Public API
 * ============================================================
 */

/**
 * Initialize UART0
 * Note: Current implementation assumes bootloader already configured UART
 *       This function is a placeholder for future full initialization
 */
void uart_init(void);

/**
 * Write a single character to UART
 * Blocks until TX FIFO is ready
 */
void uart_putc(char c);

/**
 * Write a null-terminated string to UART
 */
void uart_puts(const char *s);

/**
 * Formatted print to UART
 * 
 * Supported format specifiers:
 *   %d - signed decimal integer
 *   %u - unsigned decimal integer
 *   %x - hexadecimal (lowercase)
 *   %X - hexadecimal (uppercase)
 *   %s - null-terminated string
 *   %c - single character
 *   %% - literal '%'
 * 
 * Example:
 *   uart_printf("Value: %d, Address: 0x%08x\n", 42, 0x80000000);
 */
void uart_printf(const char *fmt, ...);

#endif /* UART_H */