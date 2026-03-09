/* ============================================================
 * main.c
 * ------------------------------------------------------------
 * Kernel Entry Point
 * ============================================================ */

#include "uart.h"
#include "watchdog.h"
#include "scheduler.h"
#include "idle.h"
#include "timer.h"
#include "irq.h"
#include "intc.h"
#include "shell.h"
#include "task.h"
#include "mmu.h"
#include <stdbool.h>

/* ============================================================
 * Shell Task (User Mode)
 * ============================================================ */
#define SHELL_STACK_SIZE 4096
static uint8_t shell_stack[SHELL_STACK_SIZE] __attribute__((aligned(4096), section(".user_stack")));
static struct task_struct shell_task;

/* ============================================================
 * MMU Test Task (User Mode)
 * ============================================================ */
extern void mmu_test_task_entry(void);
#define MMU_TEST_STACK_SIZE 4096
static uint8_t mmu_test_stack[MMU_TEST_STACK_SIZE] __attribute__((aligned(4096), section(".user_stack")));
static struct task_struct mmu_test_task;

/* ============================================================
 * Kernel Main
 * ============================================================ */
void kernel_main(void)
{
    /* 1. Hardware Init */
    watchdog_disable();
    uart_init();

    uart_printf("\n\n");
    uart_printf("========================================\n");
    uart_printf(" RefARM-OS: Interactive Shell\n");
    uart_printf("========================================\n\n");

    /* 1.5 MMU Phase B — remove identity map, update VBAR to high VA.
     * MMU was already enabled by entry.S (Phase A trampoline).
     * We are now running at VA 0xC0xxxxxx. */
    mmu_init();

    intc_init();
    irq_init();
    uart_enable_rx_interrupt(); /* Enable UART RX interrupt for keyboard input */
    timer_init();

    /* 2. Schedule Initialization */
    scheduler_init();

    /* 3. Add Idle Task (Kernel Mode) */
    struct task_struct *idle_ptr = get_idle_task();
    scheduler_add_task(idle_ptr);

    /* 4. Add Shell Task (User Mode) */
    shell_task.name = "Shell";
    shell_task.state = TASK_STATE_READY;
    shell_task.id = 0; // Will be assigned 1

    /* shells_task_entry defined in shell.h */
    task_stack_init(&shell_task, shell_task_entry, shell_stack, SHELL_STACK_SIZE);

    if (scheduler_add_task(&shell_task) < 0)
    {
        uart_printf("[BOOT] Failed to add Shell Task\n");
    }

    /* 5. Add MMU Test Task (User Mode) — validates memory protection */
    mmu_test_task.name = "MMU_Test";
    mmu_test_task.state = TASK_STATE_READY;
    mmu_test_task.id = 0;

    task_stack_init(&mmu_test_task, mmu_test_task_entry, mmu_test_stack, MMU_TEST_STACK_SIZE);

    if (scheduler_add_task(&mmu_test_task) < 0)
    {
        uart_printf("[BOOT] Failed to add MMU Test Task\n");
    }

    uart_printf("[BOOT] Starting Scheduler...\n");
    scheduler_start();

    /* Should never reach here */
    while (1)
    {
        uart_printf("PANIC: Scheduler returned!\n");
    }
}