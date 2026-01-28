/* ============================================================
 * irq_core.c
 * ------------------------------------------------------------
 * IRQ framework core implementation
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#include "irq.h"
#include "intc.h"
#include "uart.h"
#include <stddef.h>

/* ============================================================
 * IRQ Descriptor
 * ============================================================ */

struct irq_desc {
    irq_handler_t handler;  /* Handler function pointer */
    void *data;             /* Driver-specific data */
    uint32_t count;         /* Number of times handled */
};

/* ============================================================
 * IRQ Table
 * ============================================================ */

static struct irq_desc irq_table[MAX_IRQS];

/* ============================================================
 * IRQ Framework Implementation
 * ============================================================ */

void irq_init(void)
{
    /* Clear IRQ table */
    for (int i = 0; i < MAX_IRQS; i++) {
        irq_table[i].handler = NULL;
        irq_table[i].data = NULL;
        irq_table[i].count = 0;
    }
    
    uart_printf("IRQ framework initialized (max %d IRQs)\n", MAX_IRQS);
}

int irq_register_handler(uint32_t irq_num, irq_handler_t handler, void *data)
{
    /* Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        uart_printf("ERROR: Invalid IRQ number %u (max %u)\n", irq_num, MAX_IRQS - 1);
        return -1;
    }
    
    /* Validate handler */
    if (handler == NULL) {
        uart_printf("ERROR: NULL handler for IRQ %u\n", irq_num);
        return -1;
    }
    
    /* Check if already registered */
    if (irq_table[irq_num].handler != NULL) {
        uart_printf("ERROR: IRQ %u already registered\n", irq_num);
        return -1;
    }
    
    /* Register handler */
    irq_table[irq_num].handler = handler;
    irq_table[irq_num].data = data;
    irq_table[irq_num].count = 0;
    
    uart_printf("IRQ %u registered\n", irq_num);
    return 0;
}

void irq_unregister_handler(uint32_t irq_num)
{
    /* Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        return;
    }
    
    /* Clear registration (keep count for statistics) */
    irq_table[irq_num].handler = NULL;
    irq_table[irq_num].data = NULL;
    
    uart_printf("IRQ %u unregistered\n", irq_num);
}

void irq_dispatch(void *ctx)
{
    /* 
     * IRQ Dispatch Algorithm
     * See: docs/04_kernel/05_interrupt_infrastructure.md
     * Diagram: docs/04_kernel/diagram/irq_dispatch_contract.mmd
     */
    
    /* Step 1: Query INTC for active IRQ number */
    uint32_t irq_num = intc_get_active_irq();
    
    /* Step 2: Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        /* 
         * Spurious IRQ or invalid number
         * CONTRACT: Must still call EOI
         */
        uart_printf("WARNING: Spurious/Invalid IRQ %u\n", irq_num);
        intc_eoi();
        return;
    }
    
    /* Step 3: Check if handler is registered */
    if (irq_table[irq_num].handler == NULL) {
        /* 
         * Unhandled IRQ (no driver registered)
         * CONTRACT: Must still call EOI
         */
        uart_printf("WARNING: Unhandled IRQ %u\n", irq_num);
        intc_eoi();
        return;
    }
    
    /* Step 4: Call registered handler */
    irq_table[irq_num].handler(irq_table[irq_num].data);
    
    /* Step 5: Update statistics */
    irq_table[irq_num].count++;
    
    /* 
     * Step 6: Send EOI to INTC (CRITICAL CONTRACT)
     * See: docs/04_kernel/diagram/eoi_sequence.mmd
     * 
     * This MUST be called after handler completes.
     * Without EOI, INTC will not de-assert IRQ line.
     */
    intc_eoi();
}

uint32_t irq_get_count(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQS) {
        return 0;
    }
    
    return irq_table[irq_num].count;
}