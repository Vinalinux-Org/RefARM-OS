/* ============================================================
 * intc_debug.c - INTC driver with EXTENSIVE DEBUG
 * ------------------------------------------------------------
 * AM335x INTC driver
 * ============================================================
 */

#include "intc.h"
#include "mmio.h"
#include "cpu.h"

/* For debugging prints */
extern void uart_printf(const char *fmt, ...);

/* ============================================================
 * INTC Implementation with Debug Logging
 * ============================================================
 */

void intc_init(void)
{
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf("INTC INITIALIZATION START\n");
    uart_printf("========================================\n");
    
    /* Step 1: Mask all interrupts */
    uart_printf("[INTC_INIT] Step 1: Masking all interrupts...\n");
    
    uart_printf("  Bank 0 (IRQ 0-31)\n");
    mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
    
    uart_printf("  Bank 1 (IRQ 32-63)\n");
    mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
    
    uart_printf("  Bank 2 (IRQ 64-95)\n");
    mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
    
    uart_printf("  Bank 3 (IRQ 96-127)\n");
    mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);
    
    /* Verify */
    uint32_t mir0 = mmio_read32(INTC_BASE + INTC_MIR(0));
    uint32_t mir1 = mmio_read32(INTC_BASE + INTC_MIR(1));
    uint32_t mir2 = mmio_read32(INTC_BASE + INTC_MIR(2));
    uint32_t mir3 = mmio_read32(INTC_BASE + INTC_MIR(3));
    
    uart_printf("  Verify: MIR0=0x%08x, MIR1=0x%08x\n", mir0, mir1);
    uart_printf("  Verify: MIR2=0x%08x, MIR3=0x%08x\n", mir2, mir3);
    
    /* Step 2: Disable threshold */
    uart_printf("[INTC_INIT] Step 2: Disabling threshold mechanism...\n");
    mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);
    
    uint32_t threshold = mmio_read32(INTC_BASE + INTC_THRESHOLD);
    uart_printf("  THRESHOLD = 0x%08x (0xFF = disabled)\n", threshold);
    
    /* Step 3: Enable NEWIRQAGR */
    uart_printf("[INTC_INIT] Step 3: Enabling NEWIRQAGR protocol...\n");
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
    
    uint32_t control = mmio_read32(INTC_BASE + INTC_CONTROL);
    uart_printf("  CONTROL = 0x%08x (bit 0 = NEWIRQAGR)\n", control);
    
    uart_printf("========================================\n");
    uart_printf("INTC INITIALIZATION DONE\n");
    uart_printf("========================================\n\n");
}

uint32_t intc_get_active_irq(void)
{
    uint32_t sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
    
    uart_printf("[INTC_GET_ACTIVE_IRQ] SIR_IRQ = 0x%08x\n", sir);
    uart_printf("[INTC_GET_ACTIVE_IRQ] ACTIVEIRQ (bits 6:0) = %u\n", sir & 0x7F);
    uart_printf("[INTC_GET_ACTIVE_IRQ] SPURIOUS (bits 31:7) = 0x%08x\n", sir & 0xFFFFFF80);
    
    if (sir & SPURIOUSIRQ_MASK) {
        uart_printf("[INTC_GET_ACTIVE_IRQ] Spurious IRQ detected!\n");
        return 128;
    }
    
    return sir & ACTIVEIRQ_MASK;
}

void intc_eoi(void)
{
    uart_printf("[INTC_EOI] Writing NEWIRQAGR to CONTROL...\n");
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
    
    uart_printf("[INTC_EOI] Executing DSB...\n");
    dsb();
    
    uart_printf("[INTC_EOI] Complete\n");
}

void intc_enable_irq(uint32_t irq_num)
{
    uart_printf("\n[INTC_ENABLE_IRQ] Enabling IRQ %u...\n", irq_num);
    
    if (irq_num >= 128) {
        uart_printf("[INTC_ENABLE_IRQ] ERROR: Invalid IRQ number!\n");
        return;
    }
    
    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;
    
    uart_printf("[INTC_ENABLE_IRQ] Bank = %u, Bit = %u\n", bank, bit);
    uart_printf("[INTC_ENABLE_IRQ] MIR_CLEAR offset = 0x%08x\n", INTC_MIR_CLEAR(bank));
    uart_printf("[INTC_ENABLE_IRQ] Bit mask = 0x%08x\n", (1 << bit));
    
    /* Read before */
    uint32_t mir_before = mmio_read32(INTC_BASE + INTC_MIR(bank));
    uart_printf("[INTC_ENABLE_IRQ] MIR%u (before) = 0x%08x\n", bank, mir_before);
    
    /* Write to MIR_CLEAR */
    mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
    
    /* Read after */
    uint32_t mir_after = mmio_read32(INTC_BASE + INTC_MIR(bank));
    uart_printf("[INTC_ENABLE_IRQ] MIR%u (after) = 0x%08x\n", bank, mir_after);
    
    /* Verify */
    if (mir_after & (1 << bit)) {
        uart_printf("[INTC_ENABLE_IRQ] ERROR: Bit still SET (IRQ still masked!)\n");
    } else {
        uart_printf("[INTC_ENABLE_IRQ] SUCCESS: Bit CLEARED (IRQ unmasked)\n");
    }
    
    /* Check pending */
    uint32_t pending = mmio_read32(INTC_BASE + INTC_PENDING_IRQ(bank));
    uart_printf("[INTC_ENABLE_IRQ] PENDING_IRQ%u = 0x%08x\n", bank, pending);
    if (pending & (1 << bit)) {
        uart_printf("[INTC_ENABLE_IRQ] IRQ %u is PENDING!\n", irq_num);
    }
    
    /* Check ILR */
    uint32_t ilr = mmio_read32(INTC_BASE + INTC_ILR(irq_num));
    uart_printf("[INTC_ENABLE_IRQ] ILR[%u] = 0x%08x\n", irq_num, ilr);
    uart_printf("[INTC_ENABLE_IRQ]   Priority = %u\n", (ilr >> 2) & 0x3F);
    uart_printf("[INTC_ENABLE_IRQ]   FIQ/nIRQ = %u (0=IRQ, 1=FIQ)\n", ilr & 0x1);
    
    uart_printf("[INTC_ENABLE_IRQ] Done\n\n");
}

void intc_disable_irq(uint32_t irq_num)
{
    uart_printf("[INTC_DISABLE_IRQ] Disabling IRQ %u...\n", irq_num);
    
    if (irq_num >= 128) {
        return;
    }
    
    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;
    
    mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
    
    uart_printf("[INTC_DISABLE_IRQ] Done\n");
}

void intc_set_priority(uint32_t irq_num, uint32_t priority)
{
    if (irq_num >= 128 || priority > 63) {
        return;
    }
    
    uint32_t ilr = (priority & 0x3F);
    mmio_write32(INTC_BASE + INTC_ILR(irq_num), ilr);
}