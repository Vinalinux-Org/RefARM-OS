/* ============================================================
 * assert.h
 * ------------------------------------------------------------
 * Simple assertion logic for VinixOS Kernel
 * ============================================================ */

#ifndef ASSERT_H
#define ASSERT_H

#include "uart.h"

/* Assembly helper to disable IRQ */
static inline void __disable_irq(void) {
    asm volatile("cpsid i" : : : "memory", "cc");
}

/* 
 * Strict Panic Function
 * 1. Disable Interrupts (Critical!)
 * 2. Log message
 * 3. Halt CPU
 */
static inline void __attribute__((noreturn)) kernel_panic(const char *msg, const char *file, int line) {
    /* 
     * Disable IRQ and FIQ explicitly using CPS
     * 'cpsid if' disables both IRQ and FIQ
     * This works in Privileged modes (SVC, IRQ, etc.)
     */
    asm volatile("cpsid if" : : : "memory", "cc");
    
    uart_printf("\n\n[KERNEL PANIC] %s\n", msg);
    uart_printf("  AT: %s:%d\n", file, line);
    uart_printf("  SYSTEM HALTED.\n");
    
    while (1) {
        /* 
         * WFI might wake up on pending (even masked) interrupts
         * So we loop tightly.
         */
        asm volatile("wfi");
    }
}

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            kernel_panic(#cond, __FILE__, __LINE__); \
        } \
    } while (0)

#define PANIC(msg) kernel_panic(msg, __FILE__, __LINE__)

#endif /* ASSERT_H */
