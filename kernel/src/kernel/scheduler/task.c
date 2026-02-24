/* ============================================================
 * task.c
 * ------------------------------------------------------------
 * Task management implementation
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
 * Stack Design (Low <-> High):
 * [R3..R11, LR_svc]      <-- Kernel Context (Top of Stack)
 * [SPSR, Pad]            <-- Exception Frame Part 1
 * [R0..R12, PC_user]     <-- Exception Frame Part 2 (User Context)
 * 
 * Total: 10 + 2 + 14 = 26 words (104 bytes)
 * 
 * Initial register values:
 * - SPSR: SVC mode (0x13) with IRQ enabled
 * - LR_svc: svc_exit_trampoline (returns to User Mode)
 * - PC_user: Task entry point
 * - SP: points to bottom of Kernel Context (R3)
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
    /* 
     * Push in REVERSE order (R12 -> R0) so that R0 is at TOS (Low Address).
     * LDMIA loads: [SP]->R0, [SP+4]->R1 ...
     */
    for (int i = 12; i >= 0; i--) {
        *--stack_ptr = 0x0BAD0000 | i; /* Pattern: 0BAD0000, 0BAD0001... */
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
    
    /* 2.2. Push R11-R3 (9 registers: R11..R4, R3) */
    /* We include R3 to make the frame 10 words (40 bytes), keeping SP 8-byte aligned */
    for (int i = 0; i < 9; i++) {
        *--stack_ptr = 0;  /* Initial value 0 */
    }
    
    /* ============================================================
     * Finalize
     * ============================================================ */
     
     /* 
      * CRITICAL FIX: Reserve space for Kernel Stack (SVC/IRQ)
      * We must split the stack area because a shared pointer model fails:
      * - SP_svc is banked and stays at the top (empty).
      * - SP_usr grows down from the top.
      * - Exceptions using SP_svc would overwrite SP_usr data.
      * 
      * Layout:
      * [TOP]
      *   <-- Kernel Stack (Grow Down) -- SP_svc starts here
      *   ... (Reserved 512 bytes) ...
      * [BOUNDARY]
      *   <-- User Stack (Grow Down)   -- SP_usr starts here
      *   ...
      * [BOTTOM]
      */
     
    #define KERNEL_STACK_RESERVE 512
    
    /* Kernel SP points to where we pushed the context (Frame 2, which is somewhat high up).
       But wait, the Frame 1 & 2 we just pushed are for BOOTSTRAP.
       We need to ensure they are placed correctly? 
       Actually, Start_First_Task pops them.
       But we need to make sure SP_svc 'resets' to Top after pop?
       start_first_task logic:
       LDR SP, [ctx.sp] -> Points to Frame 2.
       Pop Frame 2 (SP increases).
       Pop Frame 1 (User Regs).
       BX LR.
       SP is now at Top of where Frame 1 was.
       
       We want SP_svc to handle exceptions. It should be at (Base + Size).
       So we should push frames at (Base + Size).
       
       And sets SP_usr to (Base + Size - 512).
    */
    
    task->context.sp = (uint32_t)stack_ptr;
    
    /* 
     * Initialize User Stack Pointer (SP_usr) 
     * Set to (Stack Top - Reserve) to avoid collision with Kernel Stack 
     */
    task->context.sp_usr = (uint32_t)((char *)stack_base + stack_size - KERNEL_STACK_RESERVE);
    
    /* 
     * Initialize User Link Register (LR_usr).
     */
    task->context.lr_usr = (uint32_t)entry_point;
    
    task->stack_base = stack_base;
    task->stack_size = stack_size;
    task->state = TASK_STATE_READY;
    
    /* Verify stack safety */
    if (task->context.sp <= (uint32_t)stack_base) {
        PANIC("Stack Setup Overflow");
    }
    
    /* Verify User Stack safety */
    if (task->context.sp_usr <= (uint32_t)stack_base) {
        PANIC("User Stack Setup Overflow");
    }

    uart_printf("  Stack initialized. Kernel SP=0x%08x, User SP=0x%08x\n", 
                task->context.sp, task->context.sp_usr);
    
    /* CRITICAL DEBUG: Dump stack frame */
    uart_printf("[TASK] Stack Dump:\n");
    uint32_t *d = (uint32_t *)task->context.sp;
    
    /* Frame 2 (Kernel) */
    uart_printf("  [Frame 2 - Kernel]\n");
    /* Currently SP points to R3 */
    uart_printf("  R3..R11, LR (Callee Saved + Pad):\n");
    for(int i=0; i<10; i++) uart_printf("    %08x ", *d++);
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
    uart_printf("[TASK] SP Alignment Check: 0x%08x %% 8 = %d\n", task->context.sp, task->context.sp % 8);
}
