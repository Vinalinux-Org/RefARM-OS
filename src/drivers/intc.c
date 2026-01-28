/* ============================================================
 * intc.c
 * ------------------------------------------------------------
 * AM335x INTC (Interrupt Controller) driver
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#include "intc.h"
#include "mmio.h"
#include "cpu.h"

/* ============================================================
 * INTC Driver Implementation
 * ============================================================ */

void intc_init(void)
{
    /*
     * INTC Initialization Sequence
     * See: AM335x TRM Chapter 6, Section 6.5
     * 
     * 1. Mask all interrupts
     * 2. Disable threshold mechanism
     * 3. Enable NEWIRQAGR protocol
     */
    
    /* Step 1: Mask all interrupts (set all mask bits) */
    /* Bank 0 (IRQ 0-31) */
    mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
    
    /* Bank 1 (IRQ 32-63) */
    mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
    
    /* Bank 2 (IRQ 64-95) */
    mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
    
    /* Bank 3 (IRQ 96-127) */
    mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);
    
    /* Step 2: Disable threshold mechanism (set to 0xFF) */
    mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);
    
    /* 
     * Step 3: Enable NEWIRQAGR protocol
     * Write NEWIRQAGR bit to signal initial agreement
     */
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
    
    /* 
     * Note: Priority configuration (INTC_ILR registers) not needed
     * Default priority 0 (highest) is fine for all IRQs
     */
}

uint32_t intc_get_active_irq(void)
{
    /*
     * Read INTC_SIR_IRQ register
     * Bits [6:0]: ACTIVEIRQ (IRQ number 0-127)
     * Bits [31:7]: SPURIOUSIRQ flag
     * 
     * If spurious flag is set (any bit in [31:7]), return 128
     */
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
    /*
     * End-of-Interrupt Sequence (CRITICAL)
     * See: AM335x TRM Chapter 6, Section 6.5
     * See: docs/04_kernel/diagram/eoi_sequence.mmd
     * 
     * 1. Write NEWIRQAGR to INTC_CONTROL
     * 2. Execute Data Synchronization Barrier (DSB)
     * 
     * CONTRACT:
     * - This MUST be called after handling IRQ
     * - Without EOI, INTC will NOT de-assert IRQ line
     * - CPU will immediately re-enter IRQ handler (infinite loop)
     */
    
    /* Step 1: Write NEWIRQAGR */
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
    
    /* 
     * Step 2: Data Synchronization Barrier
     * 
     * Ensures the write to INTC_CONTROL completes before
     * returning from IRQ handler.
     * 
     * Without DSB, write may be posted and EOI may not
     * complete before IRQ return, causing spurious re-entry.
     */
    dsb();
}

void intc_enable_irq(uint32_t irq_num)
{
    /*
     * Enable IRQ by clearing mask bit
     * 
     * Bank calculation:
     * - Bank 0: IRQ 0-31   (offset 0x88)
     * - Bank 1: IRQ 32-63  (offset 0xA8)
     * - Bank 2: IRQ 64-95  (offset 0xC8)
     * - Bank 3: IRQ 96-127 (offset 0xE8)
     */
    
    if (irq_num >= 128) {
        return;
    }
    
    uint32_t bank = irq_num / 32;  /* 0-3 */
    uint32_t bit = irq_num % 32;   /* 0-31 */
    
    /* Write 1 to MIR_CLEAR register to unmask (enable) IRQ */
    mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

void intc_disable_irq(uint32_t irq_num)
{
    /*
     * Disable IRQ by setting mask bit
     * 
     * Bank calculation same as enable
     */
    
    if (irq_num >= 128) {
        return;
    }
    
    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;
    
    /* Write 1 to MIR_SET register to mask (disable) IRQ */
    mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}

void intc_set_priority(uint32_t irq_num, uint32_t priority)
{
    /*
     * Set IRQ priority (optional)
     * 
     * Current implementation does not use priority.
     * All IRQs default to priority 0 (highest).
     * 
     * If needed in future:
     * - Write to INTC_ILR(irq_num) register
     * - Bits [5:0]: PRIORITY (0-63, lower = higher)
     * - Bit 0: FIQNIRQ (0=IRQ, 1=FIQ)
     */
    
    if (irq_num >= 128 || priority > 63) {
        return;
    }
    
    /* 
     * Format: [31:6]=Reserved, [5:0]=PRIORITY, [0]=FIQNIRQ
     * For IRQ routing: FIQNIRQ=0
     */
    uint32_t ilr = (priority & 0x3F);  /* Priority only, route to IRQ */
    
    mmio_write32(INTC_BASE + INTC_ILR(irq_num), ilr);
}