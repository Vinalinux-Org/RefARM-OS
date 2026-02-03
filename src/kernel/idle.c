/* ============================================================
 * idle.c
 * ------------------------------------------------------------
 * Idle task for RefARM-OS scheduler
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "task.h"
#include "uart.h"
#include "cpu.h"

/* Idle task stack (1KB) */
#define IDLE_STACK_SIZE 1024
static uint8_t idle_stack[IDLE_STACK_SIZE] __attribute__((aligned(8)));

/* Idle task structure */
static struct task_struct idle_task;

/**
 * Idle task entry point
 * Runs when no other task is ready
 */
static void idle_task_func(void)
{
    uint32_t counter = 0;
    
    uart_printf("[IDLE] Idle task started\n");
    uart_printf("[IDLE] Stack: 0x%08x - 0x%08x\n",
                (uint32_t)idle_stack,
                (uint32_t)idle_stack + IDLE_STACK_SIZE);
    
    /* Infinite loop */
    while (1) {
        /* Increment counter */
        counter++;
        
        /* Log every 1000000 iterations */
        if (counter % 1000000 == 0) {
            uart_printf("[IDLE] Running... counter=%u\n", counter);
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
    idle_task.name = "idle";
    idle_task.state = TASK_STATE_READY;
    idle_task.id = 0;  /* Will be set by scheduler */
    
    /* Initialize stack */
    task_stack_init(&idle_task, idle_task_func, idle_stack, IDLE_STACK_SIZE);
    
    return &idle_task;
}
