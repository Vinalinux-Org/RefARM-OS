/* ============================================================
 * idle.c
 * ------------------------------------------------------------
 * Idle task for RefARM-OS scheduler
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "idle.h"
#include "task.h"
#include "uart.h"
#include <stdbool.h>

/* Idle task stack (1KB) */
#define IDLE_STACK_SIZE 1024
static uint8_t idle_stack[IDLE_STACK_SIZE] __attribute__((aligned(8)));

/* Idle task structure */
static struct task_struct idle_task_struct;

// Define PRINT_INTERVAL, assuming it was 1000000 from the original code's logic
#define PRINT_INTERVAL 1000000

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
            uart_printf("[IDLE] Running... counter=%u\n", counter);
        }
        
        /* 
         * Check if scheduler wants us to yield
         * This is the cooperative yield point
         */
        extern volatile bool need_reschedule;
        if (need_reschedule) {
        if (need_reschedule) {
            /* 
             * Voluntary Yield via System Call 
             * Must use SVC #0 because we are in USR mode
             */
            __asm__ volatile("svc #0");
            
            /* ← Task resumes here after being switched back */
        }
            /* ← Task resumes here after being switched back */
        }
        
        /* 
         * WFI (Wait For Interrupt) - optional
         * Puts CPU in low power mode until interrupt
         * 
         * NOTE: Commented out for initial testing to verify
         * task switching without power management
         */
        // wfi();
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
