/* ============================================================
 * main.c
 * ------------------------------------------------------------
 * RefARM-OS: Phase 3 Verification - System Call Interface
 * Target: BeagleBone Black (AM335x)
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
#include "user_lib.h"
#include <stdbool.h>

/* ============================================================
 * Task A: Persistent Writer
 * - Tests SYS_WRITE (Loop)
 * - Tests SYS_YIELD
 * ============================================================ */
#define TASK_A_STACK_SIZE 1024
static uint8_t task_a_stack[TASK_A_STACK_SIZE] __attribute__((aligned(8), section(".user_stack")));
static struct task_struct task_a_struct;

static void task_a_entry(void)
{
    /* STATIC buffer (avoids stack issues) */
    static const char msg[] = "[TASK A] Alive! Yielding...\n";
    
    while (1) {
        /* Write to UART via Kernel */
        sys_write(msg, sizeof(msg) - 1);
        
        /* Yield to let other tasks run */
        sys_yield();
        
        /* Busy wait to slow down output (simulation) */
        for (volatile int i = 0; i < 50000; i++);
    }
}

/* ============================================================
 * Task B: One-Shot Exiter
 * - Tests SYS_WRITE (Once)
 * - Tests SYS_EXIT (Termination)
 * ============================================================ */
#define TASK_B_STACK_SIZE 1024
static uint8_t task_b_stack[TASK_B_STACK_SIZE] __attribute__((aligned(8), section(".user_stack")));
static struct task_struct task_b_struct;

static void task_b_entry(void)
{
    static const char start_msg[] = "\n[TASK B] Hello! I'm here for a good time, not a long time.\n";
    static const char exit_msg[]  = "[TASK B] Exiting now via sys_exit(42)...\n\n";

    sys_write(start_msg, sizeof(start_msg) - 1);
    
    /* Simulate some work */
    for (volatile int i = 0; i < 100000; i++);
    
    sys_write(exit_msg, sizeof(exit_msg) - 1);
    
    /* Request termination */
    sys_exit(42);
    
    /* Should never reach here */
    static const char fail_msg[] = "[TASK B] FATAL: I am still alive?!\n";
    sys_write(fail_msg, sizeof(fail_msg) - 1);
    while(1);
}

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
    uart_printf(" RefARM-OS Phase 3: Syscall Verification\n");
    uart_printf("========================================\n\n");

    intc_init();
    irq_init();
    timer_init();
    
    /* 2. Scheduler Init */
    scheduler_init();

    /* 3. Add Idle Task (Task 0) */
    struct task_struct *idle_ptr = get_idle_task();
    scheduler_add_task(idle_ptr);

    /* 4. Add Task A (Writer) */
    task_a_struct.name = "Task A (Writer)";
    task_a_struct.state = TASK_STATE_READY;
    task_a_struct.id = 0; // ID assigned by scheduler
    task_stack_init(&task_a_struct, task_a_entry, task_a_stack, TASK_A_STACK_SIZE);
    
    if (scheduler_add_task(&task_a_struct) < 0) {
        uart_printf("[BOOT] Failed to add Task A\n");
    }

    /* 5. Add Task B (Exiter) */
    task_b_struct.name = "Task B (Exiter)";
    task_b_struct.state = TASK_STATE_READY;
    task_b_struct.id = 0;
    task_stack_init(&task_b_struct, task_b_entry, task_b_stack, TASK_B_STACK_SIZE);

    if (scheduler_add_task(&task_b_struct) < 0) {
        uart_printf("[BOOT] Failed to add Task B\n");
    }

    uart_printf("[BOOT] Starting Scheduler...\n");
    scheduler_start();
    
    while(1);
}