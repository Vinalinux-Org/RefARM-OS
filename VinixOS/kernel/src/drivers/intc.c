/* ============================================================
 * intc.c
 * ------------------------------------------------------------
 * Interrupt Controller driver
 * ============================================================ */

#include "intc.h"
#include "mmio.h"
#include "cpu.h"

/* ============================================================
 * INTC Driver Implementation
 * ============================================================
 */

void intc_init(void)
{
    /* Step 1: Mask all interrupts */
    mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);
    
    /* Step 2: Disable threshold mechanism */
    mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);
    
    /* Step 3: Enable NEWIRQAGR protocol */
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
}

uint32_t intc_get_active_irq(void)
{
    uint32_t sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
    
    /* Check spurious flag */
    if (sir & SPURIOUSIRQ_MASK) {
        return 128;  /* Spurious IRQ */
    }
    
    /* Extract IRQ number */
    return sir & ACTIVEIRQ_MASK;
}

void intc_eoi(void)
{
    /* Write NEWIRQAGR to acknowledge interrupt */
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
    
    /* Data Synchronization Barrier - ensure write completes */
    dsb();
}

void intc_enable_irq(uint32_t irq_num)
{
    if (irq_num >= 128) {
        return;
    }
    
    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;
    
    /* Clear mask bit to enable IRQ */
    mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

void intc_disable_irq(uint32_t irq_num)
{
    if (irq_num >= 128) {
        return;
    }
    
    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;
    
    /* Set mask bit to disable IRQ */
    mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}

void intc_set_priority(uint32_t irq_num, uint32_t priority)
{
    if (irq_num >= 128 || priority > 63) {
        return;
    }
    
    /* Set priority, route to IRQ (not FIQ) */
    uint32_t ilr = (priority & 0x3F);
    mmio_write32(INTC_BASE + INTC_ILR(irq_num), ilr);
}