/* ============================================================
 * idle.c
 * ------------------------------------------------------------
 * Idle task for RefARM-OS scheduler
 * Target: BeagleBone Black (ARMv7-A)
 * 
 * FIXED: Phase 4 - Idle now yields on EVERY iteration
 * ============================================================ */

#include "idle.h"
#include "task.h"
#include "uart.h"
#include "user_syscall.h"
#include <stdbool.h>

/* Idle task stack (4KB) */
#define IDLE_STACK_SIZE 4096
static uint8_t idle_stack[IDLE_STACK_SIZE] __attribute__((aligned(4096), section(".user_stack")));

/* Idle task structure */
static struct task_struct idle_task_struct;

#define PRINT_INTERVAL 100000  /* Reduced for visibility during frequent context switches */

/**
 * Idle task entry point
 * Runs when no other task is ready
 */
static void idle_task(void)
{
    uint32_t counter = 0;
    
    uart_printf("[IDLE] Idle task started\n");
    uart_printf("[IDLE] Stack: 0x%08x - 0x%08x\n",
                (uint32_t)&idle_stack[0],
                (uint32_t)&idle_stack[IDLE_STACK_SIZE]);
    
    /* Main idle loop */
    while (1) {
        counter++;
        
        /* Print status periodically */
        if (counter % PRINT_INTERVAL == 0) {
            // uart_printf("[IDLE] Running... counter=%u\n", counter);
        }
        
        /* ============================================================
         * CRITICAL FIX: Always yield, not just when need_reschedule=true
         * ============================================================
         * 
         * OLD BUGGY CODE:
         *   extern volatile bool need_reschedule;
         *   if (need_reschedule) {
         *       sys_yield();  // Only yields sometimes!
         *   }
         * 
         * PROBLEM:
         * If idle only yields when need_reschedule=true, it will:
         * 1. Yield once (need_reschedule gets cleared by scheduler)
         * 2. Continue looping WITHOUT yielding
         * 3. Hog the CPU until next timer tick
         * 
         * This creates a ping-pong effect:
         * - Shell yields (sets need_reschedule=true)
         * - Idle gets scheduled
         * - Idle yields once (need_reschedule cleared)
         * - Idle loops without yielding
         * - Timer fires (sets need_reschedule=true)
         * - Idle yields again
         * - Back to Shell...
         * 
         * SOLUTION:
         * Idle should ALWAYS yield on every iteration. This is the
         * correct behavior for an idle task - it should consume
         * zero CPU when other tasks are ready.
         * 
         * The sys_yield() call will:
         * - Set need_reschedule=true internally (if not already set)
         * - Call scheduler to find next ready task
         * - If no other task is ready, come back to idle immediately
         * - If other task is ready (e.g. Shell waiting for input),
         *   switch to that task
         * ============================================================
         */
        sys_yield();
        
        /* 
         * Optional: WFI (Wait For Interrupt)
         * Uncomment to put CPU in low power mode between yields
         */
        // __asm__ volatile("wfi");
    }
}

/**
 * Get idle task structure
 * @return Pointer to idle task
 */
struct task_struct *get_idle_task(void)
{
    /* Initialize idle task */
    idle_task_struct.name = "idle";
    idle_task_struct.state = TASK_STATE_READY;
    idle_task_struct.id = 0;  /* Will be set by scheduler */
    
    /* Initialize stack */
    task_stack_init(&idle_task_struct, idle_task, idle_stack, IDLE_STACK_SIZE);
    
    return &idle_task_struct;
}