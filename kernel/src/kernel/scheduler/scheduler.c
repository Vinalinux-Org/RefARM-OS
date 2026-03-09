/* ============================================================
 * scheduler.c
 * ------------------------------------------------------------
 * Simple round-robin preemptive scheduler
 * ============================================================ */

#include "scheduler.h"
#include "task.h"
#include "uart.h"
#include "cpu.h"
#include <stddef.h>
#include <stdbool.h>
#include "trace.h"
#include "assert.h"
#include "syscalls.h" /* For process_info_t */

/* ============================================================
 * External Assembly Functions
 * ============================================================ */

/* Defined in context_switch.S */
extern void context_switch(struct task_struct *current, struct task_struct *next);
extern void start_first_task(struct task_struct *first);

/* ============================================================
 * Scheduler Data Structures
 * ============================================================ */

/* Static task array */
static struct task_struct *tasks[MAX_TASKS];
static uint32_t task_count = 0;

/* Stack Canary Value (Must match task.c) */
#define STACK_CANARY_VALUE  0xDEADBEEF

/* Current running task */
static struct task_struct *current_task = NULL;

/* Next task index for round-robin */
static uint32_t current_task_index = 0;

/* Scheduler enabled flag */
static bool scheduler_started = false;

/* ============================================================
 * Scheduler Implementation
 * ============================================================ */

/**
 * Initialize scheduler
 * Must be called before any other scheduler functions
 */
void scheduler_init(void)
{
    uart_printf("[SCHED] Initializing scheduler...\n");
    
    /* Reset task array */
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i] = NULL;
    }
    
    /* 
     * Verify structure alignment for assembly access
     * Offsets `context.sp` and `context.sp_usr` must match definition in `task_struct`.
     */
    if (__builtin_offsetof(struct task_struct, context.sp) != 52 ||
        __builtin_offsetof(struct task_struct, context.sp_usr) != 64) {
        PANIC("Struct alignment mismatch with Assembly!");
    }
    
    task_count = 0;
    current_task = NULL;
    current_task_index = 0;
    scheduler_started = false;
    
    TRACE_SCHED("Scheduler initialized (MAX_TASKS: %d)", MAX_TASKS);
}

/**
 * Add a task to scheduler
 * 
 * @param task Pointer to initialized task structure
 * @return 0 on success, -1 if task table full
 */
int scheduler_add_task(struct task_struct *task)
{
    if (task_count >= MAX_TASKS) {
        uart_printf("[SCHED] ERROR: Task table full (%d/%d)\n", 
                    task_count, MAX_TASKS);
        return -1;
    }
    
    /* Add task to array */
    tasks[task_count] = task;
    task->id = task_count;
    task->state = TASK_STATE_READY;
    
    uart_printf("[SCHED] Added task %d: '%s'\n", 
                task->id, task->name ? task->name : "(unnamed)");
    uart_printf("  Stack: 0x%08x - 0x%08x (%u bytes)\n",
                (uint32_t)task->stack_base,
                (uint32_t)task->stack_base + task->stack_size,
                task->stack_size);
    
    task_count++;
    
    return task->id;
}

/**
 * Start the scheduler
 * This function does NOT return
 * 
 * REQUIRES:
 * - At least one task added
 * - IRQ enabled in CPSR
 */
void scheduler_start(void)
{
    uart_printf("\n[SCHED] Starting scheduler...\n");
    
    if (task_count == 0) {
        uart_printf("[SCHED] ERROR: No tasks to run!\n");
        while (1);  /* Halt */
    }
    
    uart_printf("[SCHED] %d task(s) ready\n", task_count);
    
    /* Mark first task as running */
    current_task = tasks[0];
    current_task_index = 0;
    current_task->state = TASK_STATE_RUNNING;
    scheduler_started = true;
    
    uart_printf("[SCHED] Starting task 0: '%s'\n", current_task->name);
    
    TRACE_SCHED("Jumping to first task (NO RETURN)...");
    
    /* Load first task context and jump to it
     * This will:
     * - Load task's stack pointer
     * - Restore task's SPSR
     * - Restore task's registers
     * - Jump to task entry point via MOVS PC, LR
     */
    start_first_task(current_task);
    
    /* Should never reach here */
    uart_printf("[SCHED] FATAL: Returned from start_first_task!\n");
    while (1);
}

/* ============================================================
 * Global Reschedule Flag
 * ============================================================
 * 
 * Set by IRQ handler (scheduler_tick) when time slice expires.
 * Checked by tasks in their main loop.
 * Cleared when context switch completes.
 */
volatile bool need_reschedule = false;

/* ============================================================
 * scheduler_tick - Called from Timer ISR
 * ============================================================
 * 
 * CRITICAL: This runs in IRQ mode!
 * We CANNOT safely call context_switch() here because:
 * - IRQ stack is shared
 * - Nested interrupts could corrupt stack
 * 
 * Solution: Just set a flag and let tasks yield voluntarily.
 */
void scheduler_tick(void)
{
    /* Ignore ticks before scheduler starts */
    /* Ignore ticks before scheduler starts */
    if (!scheduler_started) {
        return;
    }
    
    /* 
     * IRQ-safe operation: Just set flag
     * Tasks will check this and call scheduler_yield()
     */
    need_reschedule = true;
}

/* ============================================================
 * scheduler_terminate_task - Kill a task
 * ============================================================
 * 
 * Called when a task crashes (Data/Prefetch Abort)
 */
void scheduler_terminate_task(uint32_t id)
{
    if (id >= MAX_TASKS || tasks[id] == NULL) {
        return;
    }
    
    struct task_struct *task = tasks[id];
    
    uart_printf("[SCHED] TERMINATING Task %d: '%s'\n", task->id, task->name);
    
    /* Mark as ZOMBIE */
    task->state = TASK_STATE_ZOMBIE;
    
    /* If current task is killing itself, yield immediately */
    if (current_task == task) {
        uart_printf("[SCHED] Task %d IS SUICIDE - Yielding...\n", task->id);
        
        /* 
         * CRITICAL: We are likely in ABT/UND Mode (Exception Context).
         * We MUST switch to SVC Mode before calling scheduler_yield/context_switch.
         * Otherwise, context_switch will use SP_abt/und which is wrong for the next task.
         * 
         * Logic:
         * 1. Switch to SVC Mode (keeping IRQs disabled).
         * 2. Call scheduler_yield().
         * 
         * Note: We don't care about saving the current ABT stack/regs because 
         * this task is DEAD. We just need a safe environment to switch FROM.
         */
        
        /* Switch to SVC Mode (0x13) | IRQ Disabled (0x80) | FIQ Disabled (0x40) */
        __asm__ volatile (
            "cps #0x13 \n\t"
        );
        
        need_reschedule = true; /* Force yield */
        scheduler_yield();
    }
}

/* ============================================================
 * scheduler_yield - Voluntary Task Switch
 * ============================================================
 * 
 * Called by tasks when they detect need_reschedule flag.
 * Runs in SVC mode (task context), so safe to switch.
 */
void scheduler_yield(void)
{
    struct task_struct *prev_task;
    struct task_struct *next_task;
    uint32_t next_index;
    
    // uart_printf("[SCHED] Yield called (current_task=%u)\n", 
    //             current_task ? current_task->id : 999);
    
    /* Check if reschedule is actually needed */
    if (!need_reschedule) {
        return;
    }
    
    /* Clear flag atomically */
    need_reschedule = false;
    
    /* Stack integrity check (canary) */
    if (current_task != NULL) {
        uint32_t *canary_ptr = (uint32_t *)current_task->stack_base;
        if (*canary_ptr != STACK_CANARY_VALUE) {
            TRACE_SCHED("FATAL: Stack overflow detected in task %d ('%s')", 
                        current_task->id, current_task->name);
            uart_printf("[SCHED] Canary Addr: 0x%08x. Expected: 0x%08x. Actual: 0x%08x\n",
                        (uint32_t)canary_ptr, STACK_CANARY_VALUE, *canary_ptr);
            PANIC("Stack Canary Corrupted!");
        }
    }
    
    /* Only one task? No need to switch */
    if (task_count == 1) {
        return;
    }

    /* Save pointer to current task */
    prev_task = current_task;
    
    /* Find next ready task (simple round-robin) */
    next_index = current_task_index;
    next_task = prev_task;
    uint32_t search_count = 0;
    
    // uart_printf("[SCHED] Finding next task...\n");
    
    /* Loop to find a non-ZOMBIE, READY task */
    while (search_count < MAX_TASKS) {
        next_index = (next_index + 1) % MAX_TASKS;
        search_count++;
        
        /* Debug log for search (can be noisy) */
        
        // uart_printf("[SCHED]   Task %u: %s (state=%u)\n",
        //             next_index,
        //             tasks[next_index] ? tasks[next_index]->name : "NULL",
        //             tasks[next_index] ? tasks[next_index]->state : 99);
        

        if (tasks[next_index] != NULL && 
            tasks[next_index]->state == TASK_STATE_READY) {
            // uart_printf("[SCHED]   → Selected task %u\n", next_index);
            next_task = tasks[next_index];
            break;
        }
    }

    /* Sanity check + fallback */
    if (next_task == prev_task) {
        if (prev_task->state == TASK_STATE_ZOMBIE) {
            if (tasks[0] != NULL && tasks[0]->state != TASK_STATE_ZOMBIE) {
                uart_printf("[SCHED] Warning: No READY task found, forcing idle task READY to prevent deadlock\n");
                tasks[0]->state = TASK_STATE_READY;
                next_task = tasks[0];
                next_index = 0;
            } else {
                PANIC("Scheduler Deadlock - No runnable tasks!");
            }
        } else {
            return;
        }
    }

    if (next_task->state != TASK_STATE_READY) {
        uart_printf("[SCHED] ERROR: Task %u not READY (state=%u)\n",
                    next_task->id, next_task->state);
        
        /* Fallback: Force idle to READY if it exists */
        if (tasks[0] != NULL) {
            uart_printf("[SCHED] Warning: Forcing idle task READY to prevent deadlock\n");
            tasks[0]->state = TASK_STATE_READY;
            next_task = tasks[0];
            next_index = 0;
        } else {
            PANIC("Scheduler Deadlock - No READY tasks!");
        }
    }
    
    /* Update states */
    if (prev_task->state != TASK_STATE_ZOMBIE) {
        prev_task->state = TASK_STATE_READY;
    }
    next_task->state = TASK_STATE_RUNNING;

    /* Update global scheduler state */
    current_task_index = next_index;
    current_task = next_task;
    
    // uart_printf("[SCHED] Switching: %u (%s) -> %u (%s)\n",
    //             prev_task->id, prev_task->name,
    //             next_task->id, next_task->name);
    
    /* Debug: Check Stack Depth (DISABLED for reduced noise) */
    // if (prev_task->id == 0) {
    //     uint32_t current_sp;
    //     __asm__ volatile("mov %0, sp" : "=r"(current_sp));
    //     uart_printf("[SCHED] Idle SP before switch: 0x%08x (Base: 0x%08x, Limit: 0x%08x)\n", 
    //                 current_sp, (uint32_t)prev_task->stack_base, (uint32_t)prev_task->stack_base - prev_task->stack_size);
    // }
    
    /* CRITICAL DEBUG PROBE: Inspect Shell Stack (DISABLED for reduced noise) */
    // if (next_task->id == 1) {
    //     uint32_t *sp = (uint32_t *)next_task->context.sp;
    //     uart_printf("[DEBUG] Probing Shell Stack before switch:\n");
    //     uart_printf("  SP_svc = 0x%08x\n", (uint32_t)sp);
    //     
    //     /* 
    //      * Logic:
    //      * Kernel Frame (9 words) = 36 bytes. sp[0]..sp[8]
    //      * SPSR, PAD (2 words) = 8 bytes. sp[9]..sp[10]
    //      * User Frame (14 words) = 56 bytes. sp[11]..sp[24]
    //      * PC should be at sp[24] (last popped val)
    //      */
    //     if (sp) {
    //          uart_printf("  Frame[8] (LR_svc-Trampoline) = 0x%08x\n", sp[8]);
    //          uart_printf("  Frame[9] (SPSR) = 0x%08x\n", sp[9]);
    //          uart_printf("  Frame[24] (User Entry PC) = 0x%08x\n", sp[24]);
    //          
    //          /* DUMP FULL FRAME */
    //          uart_printf("  Full Frame Dump:\n");
    //          for(int i=0; i<=24; i++) {
    //              uart_printf("    SP[%d] = 0x%08x\n", i, sp[i]);
    //          }
    //     }
    // }
    
    /* Perform context switch */
    context_switch(prev_task, next_task);
    
    /* Resumed (DISABLED for reduced noise) */
    // uart_printf("[SCHED] Resumed task %u (%s)\n", current_task->id, current_task->name);
}

/**
 * Get current running task
 * @return Pointer to current task, or NULL if scheduler not started
 */


struct task_struct *scheduler_current_task(void)
{
    return current_task;
}

/**
 * Get list of tasks
 * 
 * @param buf User buffer (array of process_info_t)
 * @param max_count Max number of entries
 * @return Number of tasks filled
 */
int scheduler_get_tasks(void *buf, uint32_t max_count)
{
    process_info_t *info = (process_info_t *)buf;
    uint32_t count = 0;
    
    /* 
     * NOTE: Buffer validation is handled by svc_handler.c's validate_user_pointer()
     * before calling this function. No additional validation needed here.
     */
    
    for (int i = 0; i < MAX_TASKS && count < max_count; i++) {
        struct task_struct *t = tasks[i];
        if (t != NULL) {
            info[count].id = t->id;
            
            /* Copy name manually (no strcpy) */
            const char *src = t->name ? t->name : "unknown";
            int copy_len = 0;
            while(src[copy_len] && copy_len < 31) {
                info[count].name[copy_len] = src[copy_len];
                copy_len++;
            }
            info[count].name[copy_len] = '\0';
            
            info[count].state = t->state;
            
            count++;
        }
    }
    
    return count;
}
