/* ============================================================
 * svc_handler.c
 * ------------------------------------------------------------
 * Supervisor Call (SVC) Exception Handler
 * Implements the system call dispatcher and user/kernel boundary
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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
    /* 
     * CRITICAL FIX: Set need_reschedule flag BEFORE calling scheduler_yield
     * 
     * Without this, voluntary yields may be ignored if the flag was already
     * cleared by a previous context switch. This causes tasks to get stuck
     * in busy-wait loops instead of properly yielding CPU to other tasks.
     * 
     * Example failure scenario without this fix:
     * 1. Shell calls sys_yield() (need_reschedule=false from previous clear)
     * 2. sys_yield() calls scheduler_yield()
     * 3. scheduler_yield() sees need_reschedule=false, returns immediately
     * 4. Shell stuck in loop, never switches to Idle
     */
    extern volatile bool need_reschedule;
    need_reschedule = true;
    
    /* Voluntary Yield */
    scheduler_yield();
    return E_OK;
}

/* sys_read(void *buf, uint32_t len) */
static int32_t sys_read(struct svc_context *ctx)
{
    void *buf = (void *)ctx->r0;
    uint32_t len = (uint32_t)ctx->r1;
    
    /* 1. Validation */
    int val_result = validate_user_pointer(buf, len);
    if (val_result != E_OK) {
        uart_printf("[SYS_READ] Validation FAILED: buf=0x%08x, len=%u, err=%d\n", 
                    (uint32_t)buf, len, val_result);
        return E_PTR;
    }
    
    if (len == 0) return 0;
    
    /* 2. Logic: Read 1 byte (NON-BLOCKING)
     * Only supporting len=1 for now (getc style) to keep it simple.
     * 
     * CRITICAL DESIGN DECISION:
     * We implement NON-BLOCKING I/O instead of blocking in kernel.
     * 
     * WHY NOT BLOCK IN KERNEL?
     * Calling scheduler_yield() from within an SVC handler is DANGEROUS:
     * - We're in exception context with SVC stack frame
     * - Context switch expects normal task stack frame
     * - Stack corruption and crashes result
     * 
     * PROPER BLOCKING I/O requires:
     * - Task sleep/wake mechanism
     * - Wait queues
     * - IRQ-driven wakeup
     * 
     * Current implementation uses NON-BLOCKING I/O:
     * - Return 0 if no data available
     * - User code must call sys_yield() and retry
     */
    char *c_buf = (char *)buf;
    int c;
    
    c = uart_getc();
    if (c == -1) {
        // uart_printf("[SYS_READ] No data, returning 0\n");
        return 0;  /* No data available - user should yield and retry */
    }
    
    // uart_printf("[SYS_READ] Got char: 0x%02x ('%c')\n", c, (c >= 32 && c <= 126) ? c : '?');
    
    /* Store result */
    *c_buf = (char)c;
    
    return 1; /* Read 1 byte */
}

/* sys_get_tasks(process_info_t *buf, uint32_t max_count) */
static int32_t sys_get_tasks(struct svc_context *ctx)
{
    void *buf = (void *)ctx->r0;
    uint32_t max_count = (uint32_t)ctx->r1;
    
    // Validate buffer (size = max_count * sizeof(process_info_t))
    uint32_t size = max_count * sizeof(process_info_t);
    if (validate_user_pointer(buf, size) != E_OK) {
        return E_PTR;
    }
    
    return scheduler_get_tasks(buf, max_count);
}

/* sys_get_meminfo(mem_info_t *buf) */
extern uint8_t _data_start[];
extern uint8_t _data_end[];
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];
extern uint8_t _stack_start[];
extern uint8_t _svc_stack_top[];
extern uint8_t _kernel_end[];

static int32_t sys_get_meminfo(struct svc_context *ctx)
{
    mem_info_t *buf = (mem_info_t *)ctx->r0;
    
    if (validate_user_pointer(buf, sizeof(mem_info_t)) != E_OK) {
        return E_PTR;
    }
    
    buf->total = 128 * 1024 * 1024; /* 128 MB */
    
    /* Calculate sizes */
    buf->kernel_text = (uint32_t)_text_end - (uint32_t)_text_start;
    buf->kernel_data = (uint32_t)_data_end - (uint32_t)_data_start;
    buf->kernel_bss  = (uint32_t)_bss_end - (uint32_t)_bss_start;
    buf->kernel_stack= (uint32_t)_svc_stack_top - (uint32_t)_stack_start;
    
    /* Free memory = Total - (Kernel End - 0x80000000) */
    /* Note: This assumes simple linear allocation */
    uint32_t kernel_end = (uint32_t)_kernel_end;
    buf->free = buf->total - (kernel_end - 0x80000000);
    
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
    
    static uint32_t svc_call_count = 0;
    svc_call_count++;
    
    /* DEBUG: Print every 5000 SVC calls */
    // if (svc_call_count % 5000 == 0) {
    //     uart_printf("[SVC] Call #%u: syscall=%u (YIELD=%u, READ=%u)\n",
    //                 svc_call_count, syscall_num, SYS_YIELD, SYS_READ);
    // }

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

        case SYS_READ:
            result = sys_read(ctx);
            break;
            
        case SYS_GET_TASKS:
            result = sys_get_tasks(ctx);
            break;
            
        case SYS_GET_MEMINFO:
            result = sys_get_meminfo(ctx);
            break;
            
        default:
            uart_printf("[SVC] ERROR: Unknown Syscall %d\n", syscall_num);
            result = E_INVAL;
            break;
    }
    
    /* Write return value back to User Context R0 */
    ctx->r0 = result;
}