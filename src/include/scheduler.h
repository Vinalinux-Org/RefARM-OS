/* ============================================================
 * scheduler.h
 * ------------------------------------------------------------
 * Simple round-robin preemptive scheduler for RefARM-OS
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "task.h"

/* ============================================================
 * Scheduler Configuration
 * ============================================================ */

#define MAX_TASKS       4       /* Maximum number of tasks */
#define IDLE_TASK_ID    0       /* Idle task always at index 0 */

/* ============================================================
 * Scheduler API
 * ============================================================ */

/**
 * Initialize scheduler subsystem
 * Must be called before any other scheduler functions
 */
void scheduler_init(void);

/**
 * Add a task to the scheduler
 * 
 * @param task Pointer to initialized task structure
 * @return 0 on success, -1 if task table full
 */
int scheduler_add_task(struct task_struct *task);

/**
 * Terminate a task (Fault Handling)
 * 
 * @param id Task ID to terminate
 */
void scheduler_terminate_task(uint32_t id);

/**
 * Start the scheduler
 * 
 * This function does NOT return. It:
 * 1. Enables timer interrupts
 * 2. Loads first task context
 * 3. Jumps to first task
 * 
 * REQUIRES: At least one task added, IRQ enabled in CPSR
 */
void scheduler_start(void) __attribute__((noreturn));

/**
 * Schedule next task (called from timer ISR)
 * 
 * Sets need_reschedule flag to signal tasks to yield.
 * Does NOT perform context switch (runs in IRQ mode).
 */
void scheduler_tick(void);

/**
 * Voluntary task yield
 * 
 * Called by tasks when they detect need_reschedule flag.
 * Performs actual context switch in SVC mode (safe).
 */
/**
 * Get list of tasks
 * @return 0 on success, -1 on failure
 */
int scheduler_get_tasks(void *buf, uint32_t max_count);

void scheduler_yield(void);

/**
 * Get current running task
 * @return Pointer to current task, or NULL if scheduler not started
 */
struct task_struct *scheduler_current_task(void);

#endif /* SCHEDULER_H */
