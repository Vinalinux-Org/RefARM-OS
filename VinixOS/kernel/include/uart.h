/* ============================================================
 * uart.h
 * ------------------------------------------------------------
 * UART driver interface
 * Target: AM335x UART0
 * ============================================================
 */

#ifndef UART_H
#define UART_H

#include "types.h"
#include <stdarg.h>

/* ============================================================
 * UART Configuration
 * ============================================================
 */

/* Ring buffer size (must be power of 2 for efficient modulo) */
#define UART_RX_BUFFER_SIZE     256

/* ============================================================
 * Public API - Transmit
 * ============================================================
 */

/**
 * Initialize UART0 (early boot)
 * 
 * Basic initialization for TX functionality only.
 * Initializes ring buffer but does NOT enable RX interrupt.
 * 
 * CONTRACT:
 * - Can be called early in boot (before IRQ framework)
 * - Only enables basic TX (uart_putc, uart_printf)
 * - Must call uart_enable_rx_interrupt() later for RX
 */
void uart_init(void);

/**
 * Enable UART RX interrupt
 * 
 * Enables RX interrupt and registers IRQ handler.
 * Must be called AFTER uart_init(), intc_init(), and irq_init().
 * 
 * CONTRACT:
 * - Must be called after intc_init() and irq_init()
 * - Must be called before irq_enable() in CPSR
 * - Registers IRQ handler with kernel
 * - Enables IRQ 72 in INTC
 */
void uart_enable_rx_interrupt(void);

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

/* ============================================================
 * Public API - Receive
 * ============================================================
 */

/**
 * Read a single character from UART (non-blocking)
 * 
 * @return Character value (0-255) on success, -1 if no data available
 * 
 * CONTRACT:
 * - Non-blocking: returns immediately
 * - Returns -1 if ring buffer is empty
 * - Safe to call from main loop or any context
 * - NOT safe to call from IRQ context
 * 
 * Example:
 *   int c = uart_getc();
 *   if (c >= 0) {
 *       uart_printf("Received: %c\n", (char)c);
 *   }
 */
int uart_getc(void);

/**
 * Check number of characters available in RX buffer
 * 
 * @return Number of unread characters (0-255)
 * 
 * CONTRACT:
 * - Returns count without consuming data
 * - Safe to call from any context
 */
int uart_rx_available(void);

/**
 * Clear RX buffer (discard all pending data)
 * 
 * CONTRACT:
 * - Resets ring buffer to empty state
 * - Discards all unread characters
 * - Safe to call from any context except IRQ
 */
void uart_rx_clear(void);

/**
 * Get UART IRQ fire count (for debugging)
 * 
 * @return Number of times UART RX IRQ handler was called
 */
uint32_t uart_get_irq_fire_count(void);

#endif /* UART_H */