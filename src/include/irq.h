/* ============================================================
 * irq.h
 * ------------------------------------------------------------
 * IRQ framework interface
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

/* ============================================================
 * IRQ Configuration
 * ============================================================ */

/* Maximum number of IRQ sources on AM335x */
#define MAX_IRQS    128

/* Common IRQ numbers (AM335x specific) */
#define IRQ_TIMER0      66
#define IRQ_TIMER1      67
#define IRQ_TIMER2      68
#define IRQ_TIMER3      69
#define IRQ_TIMER4      92
#define IRQ_TIMER5      93
#define IRQ_TIMER6      94
#define IRQ_TIMER7      95

#define IRQ_UART0       72
#define IRQ_UART1       73
#define IRQ_UART2       74
#define IRQ_UART3       44
#define IRQ_UART4       45
#define IRQ_UART5       46

#define IRQ_GPIO0A      96
#define IRQ_GPIO0B      97
#define IRQ_GPIO1A      98
#define IRQ_GPIO1B      99
#define IRQ_GPIO2A      32
#define IRQ_GPIO2B      33
#define IRQ_GPIO3A      62
#define IRQ_GPIO3B      63

/* ============================================================
 * IRQ Handler Type
 * ============================================================ */

/**
 * IRQ handler function type
 * @param data Driver-specific data passed during registration
 */
typedef void (*irq_handler_t)(void *data);

/* ============================================================
 * IRQ Framework API
 * ============================================================ */

/**
 * Initialize IRQ framework
 * Must be called before any IRQ registration
 */
void irq_init(void);

/**
 * Register IRQ handler
 * @param irq_num   IRQ number (0-127)
 * @param handler   Handler function pointer
 * @param data      Driver-specific data (passed to handler)
 * @return 0 on success, -1 on error
 * 
 * CONTRACT:
 * - irq_num must be < MAX_IRQS
 * - handler must not be NULL
 * - IRQ must not already be registered
 * - Must be called before enabling IRQ in INTC
 */
int irq_register_handler(uint32_t irq_num, irq_handler_t handler, void *data);

/**
 * Unregister IRQ handler
 * @param irq_num   IRQ number (0-127)
 * 
 * CONTRACT:
 * - IRQ should be disabled in INTC before unregistering
 */
void irq_unregister_handler(uint32_t irq_num);

/**
 * IRQ dispatcher (called from exception_entry_irq)
 * @param ctx Exception context pointer
 * 
 * CONTRACT:
 * - Queries INTC for active IRQ
 * - Validates IRQ number and handler
 * - Calls registered handler
 * - Sends EOI to INTC (ALWAYS, even for spurious/unhandled)
 */
void irq_dispatch(void *ctx);

/**
 * Get IRQ statistics
 * @param irq_num   IRQ number (0-127)
 * @return Number of times IRQ has been handled (0 if invalid)
 */
uint32_t irq_get_count(uint32_t irq_num);

#endif /* IRQ_H */