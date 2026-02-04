/* ============================================================
 * svc_handler.c
 * ------------------------------------------------------------
 * Supervisor Call (SVC) Exception Handler
 * Implements the system call dispatcher
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>
#include "assert.h"
#include "trace.h"
#include "scheduler.h"
#include "uart.h"

/* 
 * SVC Context Structure (AAPCS Aligned)
 * Matches the stack layout pushed by exception_entry_svc 
 */
struct svc_context {
    uint32_t spsr;      /* Saved Program Status Register */
    uint32_t pad;       /* Padding for 8-byte alignment */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t lr;        /* LR_svc - Points to instruction AFTER svc */
};

/* ============================================================
 * SVC Dispatcher
 * ============================================================ */
void svc_handler(struct svc_context *ctx)
{
    /* 
     * Decode the SVC number from the instruction
     * SVC Instruction: 0xEFxxxxxx (Little Endian)
     * LR in context points to next instruction (PC+4)
     * So accessing (uint32_t *)(LR - 4) gives the SVC instruction
     */
    uint32_t *svc_inst_ptr = (uint32_t *)(ctx->lr - 4);
    uint32_t svc_inst = *svc_inst_ptr;
    
    /* Extract lower 24 bits (the immediate value N in svc #N) */
    uint32_t syscall_num = svc_inst & 0x00FFFFFF;
    
    // TRACE_SCHED("SVC Trap: #%d at PC=0x%08x", syscall_num, ctx->lr - 4);

    switch (syscall_num) {
        case 0:
            /* SVC #0: scheduler_yield() */
            scheduler_yield();
            break;
            
        default:
            uart_printf("\n[SVC] FATAL: Unknown System Call #%d\n", syscall_num);
            uart_printf("[SVC] PC=0x%08x, LR=0x%08x\n", ctx->lr - 4, ctx->lr);
            /* For Phase 2, we just halt. Later we kill the task. */
            PANIC("Unknown SVC");
            break;
    }
}
