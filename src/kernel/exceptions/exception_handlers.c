/* ============================================================
 * exception_handlers.c
 * ------------------------------------------------------------
 * Exception handlers (C implementation)
 * Target: ARM Cortex-A8 (ARMv7-A)
 * ============================================================ */

#include "exception.h"
#include "uart.h"
#include "scheduler.h"
#include "task.h"
#include "assert.h"

/* ============================================================
 * Helper Functions
 * ============================================================ */

/**
 * Print exception context for debugging
 * @param ctx Exception context captured by entry stub
 */
static void print_exception_context(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("Exception Context:\n");
    uart_printf("------------------\n");
    uart_printf("PC (LR):   0x%08x\n", ctx->lr);
    uart_printf("SPSR:      0x%08x\n", ctx->spsr);
    uart_printf("\n");
    uart_printf("Registers:\n");
    uart_printf("r0:  0x%08x    r1:  0x%08x\n", ctx->r0, ctx->r1);
    uart_printf("r2:  0x%08x    r3:  0x%08x\n", ctx->r2, ctx->r3);
    uart_printf("r4:  0x%08x    r5:  0x%08x\n", ctx->r4, ctx->r5);
    uart_printf("r6:  0x%08x    r7:  0x%08x\n", ctx->r6, ctx->r7);
    uart_printf("r8:  0x%08x    r9:  0x%08x\n", ctx->r8, ctx->r9);
    uart_printf("r10: 0x%08x    r11: 0x%08x\n", ctx->r10, ctx->r11);
    uart_printf("r12: 0x%08x\n", ctx->r12);
    uart_printf("\n");
}

/**
 * Decode SPSR mode bits
 * @param spsr SPSR value
 * @return Mode string
 */
static const char* decode_cpu_mode(uint32_t spsr)
{
    switch (spsr & 0x1F) {
    case 0x10: return "USR";
    case 0x11: return "FIQ";
    case 0x12: return "IRQ";
    case 0x13: return "SVC";
    case 0x17: return "ABT";
    case 0x1B: return "UND";
    case 0x1F: return "SYS";
    default:   return "???";
    }
}

/**
 * Halt kernel with error message
 */
static void halt_kernel(void)
{
    uart_printf("\n");
    uart_printf("====================================\n");
    uart_printf("       KERNEL HALTED (FATAL)        \n");
    uart_printf("====================================\n");
    uart_printf("\n");
    
    /* Infinite loop */
    while (1)
        ;
}

/* ============================================================
 * Exception Handlers
 * ============================================================ */

/**
 * Undefined Instruction Exception Handler
 * 
 * Called when CPU encounters an invalid or unsupported instruction.
 * This is a FATAL exception in current phase - no recovery.
 */
void handle_undefined_instruction(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    uart_printf("!! UNDEFINED INSTRUCTION EXCEPTION\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    uart_printf("\n");
    uart_printf("The CPU encountered an invalid or unsupported instruction.\n");
    uart_printf("This is a FATAL exception.\n");
    
    uart_printf("\n");
    uart_printf("Possible causes:\n");
    uart_printf("  - Invalid opcode in code\n");
    uart_printf("  - VFP/NEON instruction without enabling coprocessor\n");
    uart_printf("  - Corrupted code in memory\n");
    uart_printf("  - Executing data as code\n");
    
    uart_printf("\n");
    uart_printf("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);
    
    halt_kernel();
}

/**
 * Supervisor Call (SVC) Exception Handler
 * 
 * Called when SVC instruction is executed.
 * Not used in current phase (no system calls).
 */
void handle_svc(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    uart_printf("!! SUPERVISOR CALL (SVC) EXCEPTION\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    uart_printf("\n");
    uart_printf("SVC instruction was executed.\n");
    uart_printf("System calls are NOT implemented in current phase.\n");
    
    uart_printf("\n");
    uart_printf("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);
    
    halt_kernel();
}

/**
 * Prefetch Abort Exception Handler
 * 
 * Called when instruction fetch fails.
 * This is a FATAL exception in current phase - no recovery.
 */
void handle_prefetch_abort(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    uart_printf("!! PREFETCH ABORT EXCEPTION        \n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    uart_printf("\n");
    uart_printf("Instruction fetch failed.\n");
    uart_printf("This is a FATAL exception.\n");
    
    uart_printf("\n");
    uart_printf("Possible causes:\n");
    uart_printf("  - Branching to invalid address\n");
    uart_printf("  - MMU permission fault (if MMU enabled)\n");
    uart_printf("  - Memory not present at fetch address\n");
    uart_printf("  - Alignment fault\n");
    
    uart_printf("\n");
    uart_printf("\n");
    uart_printf("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);
    
    /* FAULT CONTAINMENT LOGIC */
    if ((ctx->spsr & 0x1F) == 0x10) {
        /* User Mode Fault */
        struct task_struct *current = scheduler_current_task();
        uart_printf("\n[FAULT] User Mode Prefetch Abort DETECTED!\n");
        uart_printf("[FAULT] Offending Task: %d ('%s')\n", 
                    current ? current->id : -1, 
                    current ? current->name : "???");
        
        uart_printf("[FAULT] Action: Terminating Task...\n");
        
        if (current) {
            scheduler_terminate_task(current->id);
        } else {
            PANIC("User Fault processing failed (No Current Task)");
        }
        
        return; 
    }
    
    /* Kernel Mode Fault - PANIC */
    uart_printf("[FAULT] KERNEL PANIC: Prefetch Abort in Privileged Mode!\n");
    halt_kernel();
}

/**
 * Data Abort Exception Handler
 * 
 * Called when data access (load/store) fails.
 * This is a FATAL exception in current phase - no recovery.
 */
void handle_data_abort(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    uart_printf("!! DATA ABORT EXCEPTION            \n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    uart_printf("\n");
    uart_printf("Data access (load/store) failed.\n");
    uart_printf("This is a FATAL exception.\n");
    
    uart_printf("\n");
    uart_printf("Possible causes:\n");
    uart_printf("  - Accessing invalid memory address\n");
    uart_printf("  - MMU permission fault (if MMU enabled)\n");
    uart_printf("  - Alignment fault (unaligned access)\n");
    uart_printf("  - Writing to read-only memory\n");
    
    uart_printf("\n");
    uart_printf("\n");
    uart_printf("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);
    
    /* FAULT CONTAINMENT LOGIC */
    if ((ctx->spsr & 0x1F) == 0x10) {
        /* User Mode Fault */
        struct task_struct *current = scheduler_current_task();
        uart_printf("\n[FAULT] User Mode Data Abort DETECTED!\n");
        uart_printf("[FAULT] Offending Task: %d ('%s')\n", 
                    current ? current->id : -1, 
                    current ? current->name : "???");
        
        uart_printf("[FAULT] Action: Terminating Task...\n");
        
        if (current) {
            scheduler_terminate_task(current->id);
        } else {
            PANIC("User Fault processing failed (No Current Task)");
        }
        
        /* Execution will resume in scheduler loop after termination */
        return; 
    }
    
    /* Kernel Mode Fault - PANIC */
    uart_printf("[FAULT] KERNEL PANIC: Data Abort in Privileged Mode!\n");
    halt_kernel();
}

/**
 * IRQ Exception Handler (Stub)
 * 
 * Called when hardware interrupt occurs.
 * This should NEVER execute in current phase (IRQ masked).
 * If it does, it indicates a bug.
 */
void handle_irq(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    uart_printf("!! UNEXPECTED IRQ EXCEPTION        \n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    uart_printf("\n");
    uart_printf("IRQ exception occurred, but IRQ should be MASKED!\n");
    uart_printf("This indicates a kernel bug.\n");
    
    uart_printf("\n");
    uart_printf("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);
    
    halt_kernel();
}

/**
 * FIQ Exception Handler (Stub)
 * 
 * Called when fast interrupt occurs.
 * FIQ is NOT USED in this project.
 * If this executes, it indicates a serious bug.
 */
void handle_fiq(struct exception_context *ctx)
{
    uart_printf("\n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    uart_printf("!! UNEXPECTED FIQ EXCEPTION        \n");
    uart_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    uart_printf("\n");
    uart_printf("FIQ exception occurred, but FIQ is NOT USED!\n");
    uart_printf("This indicates a serious kernel bug.\n");
    
    uart_printf("\n");
    uart_printf("CPU Mode: %s\n", decode_cpu_mode(ctx->spsr));
    print_exception_context(ctx);
    
    halt_kernel();
}

/* ============================================================
 * Future Enhancements
 * ------------------------------------------------------------
 * 1. Read fault status registers (DFSR/IFSR) for detailed cause
 * 2. Read fault address registers (DFAR/IFAR) for faulting address
 * 3. Decode fault type (alignment, permission, translation, etc)
 * 4. Stack trace / backtrace capability
 * 5. Exception recovery for recoverable faults (future)
 * ============================================================ */