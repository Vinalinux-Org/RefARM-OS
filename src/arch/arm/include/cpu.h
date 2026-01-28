/* ============================================================
 * cpu.h
 * ------------------------------------------------------------
 * ARM CPU control interface
 * Architecture: ARMv7-A (Cortex-A8)
 * ============================================================ */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

/* ============================================================
 * CPSR Bit Definitions
 * ============================================================ */

#define CPSR_MODE_MASK  0x1F    /* Mode bits [4:0] */
#define CPSR_MODE_USR   0x10    /* User mode */
#define CPSR_MODE_FIQ   0x11    /* FIQ mode */
#define CPSR_MODE_IRQ   0x12    /* IRQ mode */
#define CPSR_MODE_SVC   0x13    /* Supervisor mode */
#define CPSR_MODE_ABT   0x17    /* Abort mode */
#define CPSR_MODE_UND   0x1B    /* Undefined mode */
#define CPSR_MODE_SYS   0x1F    /* System mode */

#define CPSR_FIQ_BIT    (1 << 6)    /* F bit - FIQ mask */
#define CPSR_IRQ_BIT    (1 << 7)    /* I bit - IRQ mask */

/* ============================================================
 * IRQ Control Functions
 * ============================================================ */

/**
 * Enable IRQ (clear I bit in CPSR)
 * 
 * CONTRACT:
 * - Must be called AFTER intc_init() and irq_init()
 * - Must have at least one handler registered (or accept spurious IRQ)
 */
static inline void irq_enable(void)
{
    uint32_t cpsr;
    asm volatile(
        "mrs %0, cpsr\n"           /* Read CPSR */
        "bic %0, %0, %1\n"         /* Clear I bit */
        "msr cpsr_c, %0"           /* Write back CPSR control field */
        : "=r"(cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
}

/**
 * Disable IRQ (set I bit in CPSR)
 * 
 * CONTRACT:
 * - Can be called at any time
 * - Prevents CPU from responding to IRQ line
 * - INTC may still assert IRQ, but CPU will ignore
 */
static inline void irq_disable(void)
{
    uint32_t cpsr;
    asm volatile(
        "mrs %0, cpsr\n"           /* Read CPSR */
        "orr %0, %0, %1\n"         /* Set I bit */
        "msr cpsr_c, %0"           /* Write back CPSR control field */
        : "=r"(cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
}

/**
 * Check if IRQ is enabled
 * @return 1 if enabled (I bit clear), 0 if disabled (I bit set)
 */
static inline int irq_is_enabled(void)
{
    uint32_t cpsr;
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    return !(cpsr & CPSR_IRQ_BIT);
}

/**
 * Save IRQ state and disable IRQ
 * @return Previous CPSR value (for restore)
 * 
 * Usage:
 *   uint32_t flags = irq_save();
 *   // ... critical section ...
 *   irq_restore(flags);
 */
static inline uint32_t irq_save(void)
{
    uint32_t cpsr, new_cpsr;
    asm volatile(
        "mrs %0, cpsr\n"
        "orr %1, %0, %2\n"
        "msr cpsr_c, %1"
        : "=r"(cpsr), "=r"(new_cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
    return cpsr;
}

/**
 * Restore IRQ state
 * @param flags Previous CPSR value from irq_save()
 */
static inline void irq_restore(uint32_t flags)
{
    asm volatile("msr cpsr_c, %0" : : "r"(flags) : "memory");
}

/* ============================================================
 * Memory Barrier Functions
 * ============================================================ */

/**
 * Data Synchronization Barrier
 * 
 * Ensures all explicit memory accesses before this instruction
 * complete before any explicit memory accesses after it
 */
static inline void dsb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory");
}

/**
 * Data Memory Barrier
 * 
 * Ensures all explicit memory accesses before this instruction
 * are observed before any explicit memory accesses after it
 */
static inline void dmb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory");
}

/**
 * Instruction Synchronization Barrier
 * 
 * Flushes the pipeline and ensures all instructions before
 * this instruction complete before fetching any instructions after it
 */
static inline void isb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c5, 4" : : "r"(0) : "memory");
}

/* ============================================================
 * Wait for Interrupt
 * ============================================================ */

/**
 * Wait for Interrupt
 * 
 * Puts the CPU into low-power state until an interrupt occurs
 * Useful in idle loop to save power
 */
static inline void wfi(void)
{
    asm volatile("wfi" : : : "memory");
}

#endif /* CPU_H */