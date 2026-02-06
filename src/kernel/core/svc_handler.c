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
#include "syscalls.h"

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

/* 
 * Validation boundaries (exported from linker script) 
 * Used to ensure user pointers are within allowed regions.
 */
extern uint8_t _user_stack_start[];
extern uint8_t _user_stack_end[];
extern uint8_t _text_start[]; /* For .rodata strings */
extern uint8_t _text_end[];

/* ============================================================
 * Helper: Validate User Pointer
 * ============================================================
 * Enforces strict memory rules:
 * Pointers must point to the User Stack region (.user_stack).
 * This sandboxes User interactions preventing Kernel corruption.
 */
static int validate_user_pointer(const void *ptr, uint32_t len)
{
    uint32_t start = (uint32_t)ptr;
    uint32_t end = start + len;
    
    /* Check for overflow */
    if (end < start) {
        return E_PTR;
    }

    uint32_t allowed_start = (uint32_t)_user_stack_start;
    uint32_t allowed_end = (uint32_t)_user_stack_end;

    /* Check bounds [start, end) within [allowed_start, allowed_end) */
    if (start >= allowed_start && end <= allowed_end) {
        return E_OK;
    }
    
    /* Allow RO Data (Code section) for static strings */
    uint32_t text_start = (uint32_t)_text_start;
    uint32_t text_end = (uint32_t)_text_end;
    if (start >= text_start && end <= text_end) {
        return E_OK;
    }
    
    return E_PTR;
}

/* ============================================================
 * Syscall Handlers
 * ============================================================ */

/* sys_write(const void *buf, uint32_t len) */
static int32_t sys_write(struct svc_context *ctx)
{
    const void *buf = (const void *)ctx->r0;
    uint32_t len = (uint32_t)ctx->r1;
    
    /* 1. Validation */
    if (validate_user_pointer(buf, len) != E_OK) {
        uart_printf("[SVC] Security Violation: Invalid Ptr 0x%08x\n", buf);
        return E_PTR;
    }
    
    /* 2. Logic (Direct Driver Call for now) */
    /* Only allow reasonable length to prevent DoS */
    if (len > 256) { 
        return E_ARG; 
    }

    /* We can safely cast because we validated range */
    const char *str = (const char *)buf;
    for (uint32_t i = 0; i < len; i++) {
        if (str[i] == '\n') {
            uart_putc('\r');
        }
        uart_putc(str[i]);
    }
    
    return (int32_t)len;
}

/* sys_exit(int status) */
static int32_t sys_exit(struct svc_context *ctx)
{
    int32_t status = (int32_t)ctx->r0;
    struct task_struct *current = scheduler_current_task();
    
    uart_printf("[SVC] Task %d exiting with status %d\n", current->id, status);
    
    /* Terminate specific task */
    scheduler_terminate_task(current->id);
    
    /* Scheduler should switch away, but if we return, it's an error flow */
    return 0; 
}

/* sys_yield() */
static int32_t sys_yield(struct svc_context *ctx)
{
    /* Voluntary Yield */
    scheduler_yield();
    return E_OK;
}

/* ============================================================
 * SVC Handler (Dispatcher)
 * ============================================================ */
void svc_handler(struct svc_context *ctx)
{
    /* 
     * ABI: 
     * R7 = Syscall Number
     * R0-R3 = Arguments
     * Return value -> R0
     */
    uint32_t syscall_num = ctx->r7;
    int32_t result = E_INVAL;

    // TRACE_SCHED("SVC Entry: ID=%d, Args=0x%x, 0x%x", syscall_num, ctx->r0, ctx->r1);

    switch (syscall_num) {
        case SYS_WRITE:
            result = sys_write(ctx);
            break;
            
        case SYS_EXIT:
            result = sys_exit(ctx);
            break;
            
        case SYS_YIELD:
            result = sys_yield(ctx);
            break;
            
        default:
            uart_printf("[SVC] ERROR: Unknown Syscall %d\n", syscall_num);
            result = E_INVAL;
            break;
    }
    
    /* Write return value back to User Context R0 */
    ctx->r0 = result;
}
