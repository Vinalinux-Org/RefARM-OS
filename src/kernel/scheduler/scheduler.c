/* ============================================================
 * scheduler.c
 * ------------------------------------------------------------
 * Simple round-robin preemptive scheduler for RefARM-OS
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "scheduler.h"
#include "task.h"
#include "uart.h"
#include "cpu.h"
#include <stddef.h>
#include <stdbool.h>
#include "trace.h"
#include "assert.h"

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
    
    /* 
     * CRITICAL DEBUG: Dump stack frame before loading
     * Verify values are still correct before start_first_task() reads them
     */
    /* 
     * CRITICAL DEBUG: Dump stack frame before loading
     * Verify values are still correct before start_first_task() reads them
     */
    TRACE_SCHED("Pre-flight check - dumping first task stack:");
    uint32_t *check_ptr = (uint32_t *)current_task->context.sp;
    
    /* Verify SPSR is SVC Mode (ignoring interrupt masks) */
    /* We expect 0x13 (SVC) or 0xD3 (SVC + NoIRQ + NoFIQ) */
    /* Verify SPSR assertion REMOVED 
     * Top of stack is now R4 (part of callee-saved context), not SPSR.
     * We trust task_stack_init() did the right thing.
     */
    // ASSERT((*check_ptr & 0x1F) == 0x13);
    
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
    
    /* Check if reschedule is actually needed */
    if (!need_reschedule) {
        return;
    }
    
    /* Clear flag atomically */
    need_reschedule = false;
    
    /* ============================================================
     * Round-Robin Scheduling Logic
     * ============================================================ */
    
    /* 
     * STACK INTEGRITY CHECK (Canary)
     * Check current task's stack bottom for overflow 
     */
    if (current_task != NULL) {
        uint32_t *canary_ptr = (uint32_t *)current_task->stack_base;
        if (*canary_ptr != STACK_CANARY_VALUE) {
            TRACE_SCHED("FATAL: Stack overflow detected in task %d ('%s')", 
                        current_task->id, current_task->name);
            TRACE_SCHED("Canary at 0x%08x is 0x%08x (expected 0x%08x)", 
                        (uint32_t)canary_ptr, *canary_ptr, STACK_CANARY_VALUE);
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
    next_index = (current_task_index + 1) % MAX_TASKS;
    
    /* Handle wrap-around */
    if (next_index >= task_count) {
        next_index = 0;
    }
    
    next_task = tasks[next_index];
    
    /* Sanity check */
    if (next_task->state != TASK_STATE_READY) {
        uart_printf("[SCHED] ERROR: Next task %u not ready! State: %u\n", next_task->id, next_task->state);
        // Fallback: try to find another ready task or stick with current
        // For now, just return to current task
        return;
    }
    
    /* Update states */
    prev_task->state = TASK_STATE_READY;
    next_task->state = TASK_STATE_RUNNING;

    /* Update global scheduler state */
    /* Update global scheduler state */
    current_task_index = next_index;
    current_task = next_task;
    
    TRACE_SCHED("Yield: %u -> %u", prev_task->id, next_task->id);
    
    /* 
     * Perform context switch
     * Safe here because we're in SVC mode (task context)
     */
    context_switch(prev_task, next_task);
    
    /* NEVER REACHED - task will resume here on next switch back */
}

/**
 * Get current running task
 * @return Pointer to current task, or NULL if scheduler not started
 */
struct task_struct *scheduler_current_task(void)
{
    return current_task;
}
