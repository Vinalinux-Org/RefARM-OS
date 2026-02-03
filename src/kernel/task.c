/* ============================================================
 * task.c
 * ------------------------------------------------------------
 * Task management implementation for RefARM-OS
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "task.h"
#include "cpu.h"
#include "uart.h"
#include <stddef.h>

/* ============================================================
 * Task Stack Initialization
 * ============================================================ */

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
void task_stack_init(struct task_struct *task, 
                     void (*entry_point)(void),
                     void *stack_base, 
                     uint32_t stack_size)
{
    uint32_t *stack_ptr;
    uint32_t initial_spsr;
    
    uart_printf("[TASK] Initializing stack for task '%s'\n", 
                task->name ? task->name : "(unnamed)");
    
    /* 
     * Stack setup:
     * stack_base points to HIGH address (stack top)
     * We build frame from top downwards
     */
    stack_ptr = (uint32_t *)((char *)stack_base + stack_size);
    
    uart_printf("  Stack base: 0x%08x\n", (uint32_t)stack_base);
    uart_printf("  Stack size: %u bytes\n", stack_size);
    uart_printf("  Stack top:  0x%08x\n", (uint32_t)stack_ptr);
    uart_printf("  Entry point: 0x%08x\n", (uint32_t)entry_point);
    
    /* 
     * Build initial stack frame
     * 
     * CRITICAL: Must match EXACTLY what start_first_task() expects!
     * 
     * start_first_task() pops in this order (Low -> High):
     *   1. SPSR  (first pop)
     *   2. LR    (second pop)
     *   3. r0-r12 (third pop)
     * 
     * Stack grows DOWN, so we must PUSH in REVERSE order:
     *   1st Push: r0-r12 (Highest Address)
     *   2nd Push: LR
     *   3rd Push: SPSR (Lowest Address)
     */
    
    /* 1. Push r0-r12 FIRST (13 registers, all zeros) */
    /* These will be at HIGHEST addresses in the frame */
    for (int i = 0; i < 13; i++) {
        *--stack_ptr = 0;  /* Decrement first, then write */
    }
    
    /* 2. Push LR SECOND (entry point) */
    *--stack_ptr = (uint32_t)entry_point;
    
    /* 
     * 3. Push SPSR LAST (will be at LOWEST address)
     * 
     * Initial processor state for task:
     * - Mode: SVC (0x13)
     * - I bit: 0 (IRQ enabled)
     * - F bit: 0 (FIQ enabled)
     */
    initial_spsr = CPSR_MODE_SVC;  /* 0x13 */
    if (task->id == 0) {
        /* SPECIAL: Indirect way to keep IRQs disabled for idle task initially?
           No, keep 0x13. start_first_task enables IRQ via SPSR restore. */
    }
    *--stack_ptr = initial_spsr;
    
    /* 
     * Stack frame complete!
     * 
     * Layout (low → high address):
     * [SPSR] ← SP points here (will be popped FIRST)
     * [LR]
     * [r0-r12]
     */
    task->context.sp = (uint32_t)stack_ptr;
    
    /* Fill in task metadata */
    task->stack_base = stack_base;
    task->stack_size = stack_size;
    task->state = TASK_STATE_READY;
    
    /* Log final state */
    uart_printf("  Initial SP: 0x%08x (after frame setup)\n", 
                task->context.sp);
    uart_printf("  Frame size: %u bytes\n", 
                (uint32_t)stack_base + stack_size - task->context.sp);
    uart_printf("  Initial SPSR: 0x%08x (SVC mode, IRQ enabled)\n", 
                initial_spsr);
    
    
    /* Verify stack didn't overflow */
    if (task->context.sp < (uint32_t)stack_base) {
        uart_printf("  [ERROR] Stack overflow during init!\n");
        uart_printf("    SP went below stack_base\n");
        while (1);  /* Halt */
    }
    
    uart_printf("  Stack initialized successfully\n");
    
    /* 
     * CRITICAL DEBUG: Dump actual stack memory
     * Verify values are written correctly
     */
    uart_printf("\n[TASK] Memory dump of stack frame:\n");
    uint32_t *dump_ptr = (uint32_t *)task->context.sp;
    
    /* Stack expands HIGH relative to SP */
    uart_printf("  [0x%08x] SPSR = 0x%08x (expect 0x00000013)\n", 
                (uint32_t)dump_ptr, *dump_ptr);
    dump_ptr++;
    uart_printf("  [0x%08x] LR   = 0x%08x (expect 0x%08x)\n",
                (uint32_t)dump_ptr, *dump_ptr, (uint32_t)entry_point);
    
    /* Dump first few r registers */
    for (int i = 12; i >= 0; i--) {
        dump_ptr++;
        if (i >= 10) {
            uart_printf("  [0x%08x] r%d  = 0x%08x (expect 0x00000000)\n",
                        (uint32_t)dump_ptr, i, *dump_ptr);
        }
    }
    
    uart_printf("\n");
}
