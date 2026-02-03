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

/* Current running task */
static struct task_struct *current_task = NULL;

/* Next task index for round-robin */
static uint32_t current_task_index = 0;

/* Scheduler enabled flag */
static uint32_t scheduler_running = 0;

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
    scheduler_running = 0;
    
    uart_printf("[SCHED] Scheduler initialized\n");
    uart_printf("  MAX_TASKS: %d\n", MAX_TASKS);
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
    scheduler_running = 1;
    
    uart_printf("[SCHED] Starting task 0: '%s'\n", current_task->name);
    
    /* 
     * CRITICAL DEBUG: Dump stack frame before loading
     * Verify values are still correct before start_first_task() reads them
     */
    uart_printf("[SCHED] Pre-flight check - dumping first task stack:\n");
    uint32_t *check_ptr = (uint32_t *)current_task->context.sp;
    uart_printf("  task->context.sp = 0x%08x\n", current_task->context.sp);
    uart_printf("  [0x%08x] = 0x%08x (SPSR, expect 0x13)\n", 
                (uint32_t)check_ptr, *check_ptr);
    check_ptr++;
    uart_printf("  [0x%08x] = 0x%08x (LR, expect task entry)\n",
                (uint32_t)check_ptr, *check_ptr);
    check_ptr++;
    uart_printf("  [0x%08x] = 0x%08x (r12)\n",
                (uint32_t)check_ptr, *check_ptr);
    
    uart_printf("[SCHED] Jumping to first task (NO RETURN)...\n\n");
    
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

/**
 * Schedule next task (called from timer ISR)
 * Performs round-robin context switch
 */
void scheduler_tick(void)
{
    struct task_struct *prev_task;
    struct task_struct *next_task;
    uint32_t next_index;
    
    /* Paranoid check */
    if (!scheduler_running || current_task == NULL) {
        uart_printf("[SCHED] WARNING: Tick before scheduler started\n");
        return;
    }
    
    /* Only one task? No need to switch */
    if (task_count == 1) {
        return;  /* Continue running current task */
    }
    
    /* Round-robin: select next task */
    next_index = (current_task_index + 1) % task_count;
    next_task = tasks[next_index];
    
    /* Same task? (shouldn't happen with count > 1) */
    if (next_task == current_task) {
        return;
    }
    
    /* Save pointer to current task (about to become previous) */
    prev_task = current_task;
    
    /* Update states */
    prev_task->state = TASK_STATE_READY;
    next_task->state = TASK_STATE_RUNNING;
    
    /* 
     * CRITICAL: Update scheduler state BEFORE context switch
     * 
     * context_switch() NEVER RETURNS (it's an exception return)
     * When this task runs again, it will return from a DIFFERENT
     * scheduler_tick() call (from a future timer interrupt)
     * 
     * Therefore, we must update current_task BEFORE switching,
     * so the next tick sees correct state.
     */
    current_task = next_task;
    current_task_index = next_index;
    
    /* Perform context switch
     * This will:
     * - Save prev_task's registers to its stack
     * - Load next_task's registers from its stack
     * - Jump to next_task via MOVS PC, LR (exception return)
     * 
     * ❌ CODE AFTER THIS POINT IS NEVER EXECUTED
     * When current task runs again, it continues from where
     * it was preempted, NOT from here.
     */
    // context_switch(prev_task, next_task);
    
    /* TEMPORARY DEBUG: Just switch pointers back to fake it */
    current_task = prev_task;
    current_task_index = (next_index + MAX_TASKS - 1) % MAX_TASKS;
    uart_printf("[SCHED] Tick! (Context switch skipped for debug)\n");
    return;
    
    /* ❌ DEAD CODE - never reached */
}

/**
 * Get current running task
 * @return Pointer to current task, or NULL if scheduler not started
 */
struct task_struct *scheduler_current_task(void)
{
    return current_task;
}
