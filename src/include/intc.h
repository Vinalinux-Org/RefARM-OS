/* ============================================================
 * intc.h
 * ------------------------------------------------------------
 * AM335x INTC (Interrupt Controller) driver interface
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#ifndef INTC_H
#define INTC_H

#include <stdint.h>

/* ============================================================
 * INTC Hardware Definitions
 * ============================================================ */

/* INTC Base Address */
#define INTC_BASE           0x48200000

/* INTC Control Registers */
#define INTC_REVISION       0x00
#define INTC_SYSCONFIG      0x10
#define INTC_SYSSTATUS      0x14
#define INTC_SIR_IRQ        0x40
#define INTC_SIR_FIQ        0x44
#define INTC_CONTROL        0x48
#define INTC_PROTECTION     0x4C
#define INTC_IDLE           0x50
#define INTC_IRQ_PRIORITY   0x60
#define INTC_FIQ_PRIORITY   0x64
#define INTC_THRESHOLD      0x68

/* INTC Bank Registers (4 banks, 32 IRQs each) */
#define INTC_ITR(n)         (0x80 + ((n) * 0x20))
#define INTC_MIR(n)         (0x84 + ((n) * 0x20))
#define INTC_MIR_CLEAR(n)   (0x88 + ((n) * 0x20))
#define INTC_MIR_SET(n)     (0x8C + ((n) * 0x20))
#define INTC_ISR_SET(n)     (0x90 + ((n) * 0x20))
#define INTC_ISR_CLEAR(n)   (0x94 + ((n) * 0x20))
#define INTC_PENDING_IRQ(n) (0x98 + ((n) * 0x20))
#define INTC_PENDING_FIQ(n) (0x9C + ((n) * 0x20))

/* INTC ILR registers (priority + FIQ/IRQ routing) */
#define INTC_ILR(m)         (0x100 + ((m) * 4))

/* INTC_SIR_IRQ bit fields */
#define ACTIVEIRQ_MASK      0x7F        /* Bits 6:0 */
#define SPURIOUSIRQ_MASK    0xFFFFFF80  /* Bits 31:7 */

/* INTC_CONTROL bits */
#define NEWIRQAGR           (1 << 0)
#define NEWFIQAGR           (1 << 1)

/* ============================================================
 * INTC Driver API
 * ============================================================ */

/**
 * Initialize INTC
 * - Masks all interrupts
 * - Disables threshold mechanism
 * - Enables NEWIRQAGR protocol
 * 
 * Must be called before any IRQ usage
 */
void intc_init(void);

/**
 * Get active IRQ number
 * @return IRQ number (0-127), or 128 if spurious
 * 
 * CONTRACT:
 * - Reads INTC_SIR_IRQ register
 * - Checks SPURIOUSIRQ flag
 * - Returns 128 for spurious IRQ
 */
uint32_t intc_get_active_irq(void);

/**
 * End-of-Interrupt sequence
 * 
 * CONTRACT (CRITICAL):
 * - MUST be called after handling IRQ
 * - MUST be called for ALL IRQs (valid, spurious, unhandled)
 * - Without EOI, INTC will not de-assert IRQ line
 * 
 * Implementation:
 * - Writes NEWIRQAGR to INTC_CONTROL
 * - Executes Data Synchronization Barrier (DSB)
 */
void intc_eoi(void);

/**
 * Enable specific IRQ
 * @param irq_num IRQ number (0-127)
 * 
 * CONTRACT:
 * - Handler must be registered before enabling
 * - Clears mask bit in MIR_CLEAR register
 */
void intc_enable_irq(uint32_t irq_num);

/**
 * Disable specific IRQ
 * @param irq_num IRQ number (0-127)
 * 
 * CONTRACT:
 * - Should be called before unregistering handler
 * - Sets mask bit in MIR_SET register
 */
void intc_disable_irq(uint32_t irq_num);

/**
 * Set IRQ priority (optional)
 * @param irq_num   IRQ number (0-127)
 * @param priority  Priority (0-63, lower = higher priority)
 * 
 * Note: Current implementation does not use priority
 * All IRQs default to priority 0
 */
void intc_set_priority(uint32_t irq_num, uint32_t priority);

#endif /* INTC_H */