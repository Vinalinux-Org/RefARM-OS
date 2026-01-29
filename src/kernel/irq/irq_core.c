/* ============================================================
 * irq_core_debug.c - IRQ framework with EXTENSIVE DEBUG
 * ------------------------------------------------------------
 * Target: AM335x / BeagleBone Black
 * ============================================================
 */

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

/* Debug counter */
static volatile uint32_t irq_dispatch_count = 0;

/* ============================================================
 * IRQ Framework Implementation
 * ============================================================
 */

void irq_init(void)
{
    for (int i = 0; i < MAX_IRQS; i++) {
        irq_table[i].handler = NULL;
        irq_table[i].data = NULL;
        irq_table[i].count = 0;
    }
    
    uart_printf("IRQ framework initialized (max %d IRQs)\n", MAX_IRQS);
}

int irq_register_handler(uint32_t irq_num, irq_handler_t handler, void *data)
{
    if (irq_num >= MAX_IRQS) {
        uart_printf("ERROR: Invalid IRQ number %u (max %u)\n", irq_num, MAX_IRQS - 1);
        return -1;
    }
    
    if (handler == NULL) {
        uart_printf("ERROR: NULL handler for IRQ %u\n", irq_num);
        return -1;
    }
    
    if (irq_table[irq_num].handler != NULL) {
        uart_printf("ERROR: IRQ %u already registered\n", irq_num);
        return -1;
    }
    
    irq_table[irq_num].handler = handler;
    irq_table[irq_num].data = data;
    irq_table[irq_num].count = 0;
    
    uart_printf("IRQ %u registered\n", irq_num);
    return 0;
}

void irq_unregister_handler(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQS) {
        return;
    }
    
    irq_table[irq_num].handler = NULL;
    irq_table[irq_num].data = NULL;
    
    uart_printf("IRQ %u unregistered\n", irq_num);
}

void irq_dispatch(void *ctx)
{
    irq_dispatch_count++;
    
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf("[IRQ_DISPATCH] Entry #%u\n", irq_dispatch_count);
    uart_printf("========================================\n");
    
    /* Step 1: Query INTC for active IRQ */
    uart_printf("[IRQ_DISPATCH] Querying INTC for active IRQ...\n");
    uint32_t irq_num = intc_get_active_irq();
    
    uart_printf("[IRQ_DISPATCH] Active IRQ number = %u\n", irq_num);
    
    /* Step 2: Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        uart_printf("[IRQ_DISPATCH] WARNING: Spurious/Invalid IRQ %u\n", irq_num);
        uart_printf("[IRQ_DISPATCH] Calling EOI...\n");
        intc_eoi();
        uart_printf("[IRQ_DISPATCH] Exit (spurious)\n");
        uart_printf("========================================\n\n");
        return;
    }
    
    /* Step 3: Check handler */
    if (irq_table[irq_num].handler == NULL) {
        uart_printf("[IRQ_DISPATCH] WARNING: Unhandled IRQ %u\n", irq_num);
        uart_printf("[IRQ_DISPATCH] No handler registered!\n");
        uart_printf("[IRQ_DISPATCH] Calling EOI...\n");
        intc_eoi();
        uart_printf("[IRQ_DISPATCH] Exit (unhandled)\n");
        uart_printf("========================================\n\n");
        return;
    }
    
    /* Step 4: Call handler */
    uart_printf("[IRQ_DISPATCH] Calling handler for IRQ %u...\n", irq_num);
    irq_table[irq_num].handler(irq_table[irq_num].data);
    uart_printf("[IRQ_DISPATCH] Handler returned\n");
    
    /* Step 5: Update statistics */
    irq_table[irq_num].count++;
    uart_printf("[IRQ_DISPATCH] IRQ %u count = %u\n", irq_num, irq_table[irq_num].count);
    
    /* Step 6: EOI */
    uart_printf("[IRQ_DISPATCH] Calling EOI...\n");
    intc_eoi();
    uart_printf("[IRQ_DISPATCH] EOI complete\n");
    
    uart_printf("[IRQ_DISPATCH] Exit (handled)\n");
    uart_printf("========================================\n\n");
}

uint32_t irq_get_count(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQS) {
        return 0;
    }
    
    return irq_table[irq_num].count;
}