/* ============================================================
 * task.c
 * ------------------------------------------------------------
 * Task management implementation for RefARM-OS
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "task.h"
#include "cpu.h"
#include "uart.h"
#include "assert.h"
#include <stddef.h>

/* ============================================================
 * Task Stack Initialization
 * ============================================================ */

/* Magic number for stack overflow detection */
#define STACK_CANARY_VALUE  0xDEADBEEF

/**
 * Initialize task stack for first-time execution
 * 
 * Creates initial stack frame so context_switch() can load the task.
 * Stack frame must match EXACTLY what context_switch.S expects.
 * 
 * Per Phase 7 Design:
 * - Stack grows DOWN (from high to low address)
 * - Frame layout (low → high addr): [SPSR] [LR] [r0-r12]
 * - Total: 60 bytes (15 words)
 * 
 * Initial register values:
 * - SPSR: SVC mode (0x13) with IRQ enabled (I bit = 0)
 * - LR: entry_point (task will start here)
 * - r0-r12: all zeros
 * - SP: points to bottom of frame (lowest address)
 */
extern void svc_exit_trampoline(void);

void task_stack_init(struct task_struct *task, 
                     void (*entry_point)(void),
                     void *stack_base, 
                     uint32_t stack_size)
{
    uint32_t *stack_ptr;
    
    uart_printf("[TASK] Initializing stack for task '%s' (User Mode Model)\n", 
                task->name ? task->name : "(unnamed)");
    
    /* STACK CANARY: Write magic number at stack bottom */
    *(uint32_t *)stack_base = STACK_CANARY_VALUE;
    
    stack_ptr = (uint32_t *)((char *)stack_base + stack_size);
    
    /* ============================================================
     * FRAME 1: USER CONTEXT (High Address)
     * This frame simulates the state saved by SVC Handler logic.
     * svc_exit_trampoline will pop this to return to User Mode.
     * ============================================================ */
    
    /* 1.1. Push LR (User Entry Point) */
    *--stack_ptr = (uint32_t)entry_point;
    
    /* 1.2. Push R0-R12 (User Registers) */
    for (int i = 0; i < 13; i++) {
        *--stack_ptr = 0;
    }
    
    /* 1.3. Push Padding (Alignment) */
    *--stack_ptr = 0;
    
    /* 1.4. Push SPSR (USR Mode = 0x10) */
    *--stack_ptr = 0x10; 
    
    /* ============================================================
     * FRAME 2: KERNEL CONTEXT (Low Address)
     * This frame simulates the state saved by context_switch().
     * Structure: [LR] [R11] ... [R4] (Callee-Saved Only)
     * start_first_task() will pop this to "return" to trampoline.
     * ============================================================ */
    
    /* 2.1. Push LR (Return address -> Trampoline) */
    *--stack_ptr = (uint32_t)svc_exit_trampoline;
    
    /* 2.2. Push R11-R4 (8 registers) */
    for (int i = 0; i < 8; i++) {
        *--stack_ptr = 0;  /* Initial value 0 */
    }
    
    /* ============================================================
     * Finalize
     * ============================================================ */
    task->context.sp = (uint32_t)stack_ptr;
    
    /* 
     * CORRECTION: Initialize User Stack Pointer (SP_usr) 
     * SPSR is set to User Mode. When we return to User Mode, 
     * SP_usr must point to valid stack. 
     * We share the same stack area. The User Stack starts where Kernel Stack ends.
     */
    task->context.sp_usr = (uint32_t)stack_ptr;
    
    task->stack_base = stack_base;
    task->stack_size = stack_size;
    task->state = TASK_STATE_READY;
    
    /* Verify stack safety */
    if (task->context.sp <= (uint32_t)stack_base) {
        PANIC("Stack Setup Overflow");
    }
    
    uart_printf("  Stack initialized. SP=0x%08x\n", task->context.sp);
    
    /* CRITICAL DEBUG: Dump stack frame */
    uart_printf("[TASK] Stack Dump:\n");
    uint32_t *d = (uint32_t *)task->context.sp;
    
    /* Frame 2 (Kernel) */
    uart_printf("  [Frame 2 - Kernel]\n");
    /* Currently SP points to R4 */
    uart_printf("  R4..R11, LR (Callee Saved):\n");
    for(int i=0; i<9; i++) uart_printf("    %08x ", *d++);
    uart_printf("\n");
    
    /* Frame 1 (User) */
    uart_printf("  [Frame 1 - User]\n");
    uart_printf("  SPSR: 0x%08x (Expected 0x10)\n", *d++);
    uart_printf("  PAD:  0x%08x\n", *d++);
    uart_printf("  R12..R0 (User):\n");
    for(int i=0; i<13; i++) uart_printf("    %08x ", *d++);
    uart_printf("\n");
    uart_printf("  LR:   0x%08x (Expected Entry Point)\n", *d++);
    
    /* Check entry_point val */
    uart_printf("[TASK] Entry Point Addr: 0x%08x\n", (uint32_t)entry_point);
}
