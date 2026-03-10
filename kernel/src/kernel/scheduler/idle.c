/* ============================================================
 * idle.c
 * ------------------------------------------------------------
 * Idle Task Implementation
 * Runs when no other task is ready. Loops and yields.
 * ============================================================ */

#include "idle.h"
#include "task.h"
#include "scheduler.h"
#include <stdbool.h>

/* Idle task stack (4KB) */
#define IDLE_STACK_SIZE 4096
static uint8_t idle_stack[IDLE_STACK_SIZE] __attribute__((aligned(4096), section(".user_stack")));

/* Idle task structure */
static struct task_struct idle_task_struct;

#define PRINT_INTERVAL 100000 /* Reduced for visibility during frequent context switches */

/**
 * Idle task entry point
 * Runs when no other task is ready
 */
static void idle_task(void)
{
    uint32_t counter = 0;

    /* Main idle loop */
    while (1)
    {
        counter++;

        /* Print status periodically */
        if (counter % PRINT_INTERVAL == 0)
        {
            // uart_printf("[IDLE] Running... counter=%u\n", counter);
        }

        /* ============================================================
         * Idle always requests reschedule before yielding.
         * scheduler_yield() is gated by need_reschedule flag.
         * Without setting it here, idle returns immediately from
         * scheduler_yield() and busy-loops until the next timer
         * tick (10ms), causing visible input/output lag in shell.
         * ============================================================ */
        extern volatile bool need_reschedule;
        need_reschedule = true;
        scheduler_yield();

        /*
         * WFI (Wait For Interrupt)
         * If scheduler_yield returned without switching (no other
         * READY tasks), sleep until next interrupt (timer/UART RX).
         * This avoids burning CPU when truly idle.
         */
        __asm__ volatile("wfi");
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
    idle_task_struct.id = 0; /* Will be set by scheduler */

    /* Initialize stack */
    task_stack_init(&idle_task_struct, idle_task, idle_stack, IDLE_STACK_SIZE);

    return &idle_task_struct;
}