/* ============================================================
 * irq_core.c
 * ------------------------------------------------------------
 * IRQ framework core implementation
 * ============================================================ */

#include "irq.h"
#include "intc.h"
#include "uart.h"
#include <stddef.h>

/* ============================================================
 * IRQ Descriptor
 * ============================================================
 */

struct irq_desc {
    irq_handler_t handler;
    void *data;
    uint32_t count;
};

/* ============================================================
 * IRQ Table
 * ============================================================
 */

static struct irq_desc irq_table[MAX_IRQS];

/* ============================================================
 * IRQ Framework Implementation
 * ============================================================
 */

void irq_init(void)
{
    /* Clear IRQ table */
    for (int i = 0; i < MAX_IRQS; i++) {
        irq_table[i].handler = NULL;
        irq_table[i].data = NULL;
        irq_table[i].count = 0;
    }
}

int irq_register_handler(uint32_t irq_num, irq_handler_t handler, void *data)
{
    /* Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        return -1;
    }
    
    /* Validate handler */
    if (handler == NULL) {
        return -1;
    }
    
    /* Check if already registered */
    if (irq_table[irq_num].handler != NULL) {
        return -1;
    }
    
    /* Register handler */
    irq_table[irq_num].handler = handler;
    irq_table[irq_num].data = data;
    irq_table[irq_num].count = 0;
    
    return 0;
}

void irq_unregister_handler(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQS) {
        return;
    }
    
    /* Clear registration (keep count for statistics) */
    irq_table[irq_num].handler = NULL;
    irq_table[irq_num].data = NULL;
}

void irq_dispatch(void *ctx)
{
    /* Query INTC for active IRQ number */
    uint32_t irq_num = intc_get_active_irq();
    
    /* Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        /* Spurious or invalid IRQ - still must send EOI */
        intc_eoi();
        return;
    }
    
    /* Check if handler is registered */
    if (irq_table[irq_num].handler == NULL) {
        /* Unhandled IRQ - still must send EOI */
        intc_eoi();
        return;
    }
    
    /* Call registered handler */
    irq_table[irq_num].handler(irq_table[irq_num].data);
    
    /* Update statistics */
    irq_table[irq_num].count++;
    
    /* Send EOI to INTC (CRITICAL - must always be called) */
    intc_eoi();
}

uint32_t irq_get_count(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQS) {
        return 0;
    }
    
    return irq_table[irq_num].count;
}