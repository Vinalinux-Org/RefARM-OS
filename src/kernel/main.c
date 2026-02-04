/* ============================================================
 * main.c
 * ------------------------------------------------------------
 * RefARM-OS: Scheduler - Two Task Context Switch Test
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
#include <stdbool.h>

/* ============================================================
 * Test Task Logic (Integrated)
 * ============================================================ */

/* Test task stack (1KB) */
#define TEST_STACK_SIZE 1024
#define PRINT_INTERVAL 1000000
static uint8_t test_stack[TEST_STACK_SIZE] __attribute__((aligned(8)));

/* Test task structure */
static struct task_struct test_task_struct;

/**
 * Test task entry point
 */
/* ============================================================
 * User Space Wrapper
 * ============================================================ */
void syscall_yield(void) 
{
    /* SVC #0 - Yield System Call */
    __asm__ volatile("svc #0");
}

/**
 * Test task entry point
 * Now runs in USER MODE (PL0)
 */
static void test_task(void)
{
    uint32_t counter = 0;
    
    uart_printf("[TEST] Test task started (USR Mode)\n");
    uart_printf("[TEST] This task should NOT be able to access CPSR M-bits!\n");
    
    /* Main test loop */
    while (1) {
        counter++;
        
        /* Print status periodically */
        if (counter % (PRINT_INTERVAL/10) == 0) { // Speed up for test
            uart_printf("[TEST] User Task Running... (Counter %u)\n", counter);
             
            /* Voluntary Yield via Syscall */
            syscall_yield();
        }
    }
}

/**
 * Get test task structure
 */
static struct task_struct *create_test_task(void)
{
    test_task_struct.name = "test";
    test_task_struct.state = TASK_STATE_READY;
    test_task_struct.id = 0;
    
    task_stack_init(&test_task_struct, test_task, test_stack, TEST_STACK_SIZE);
    
    return &test_task_struct;
}

void kernel_main(void)
{
    /* ========================================================
     * Phase 0: Early boot
     * ======================================================== */
    watchdog_disable();
    uart_init();

    uart_printf("\n\n");
    uart_printf("========================================\n");
    uart_printf(" RefARM-OS Booting...\n");
    uart_printf(" Phase 7: Two Task Test\n");
    uart_printf(" Context Switch Verification\n");
    uart_printf("========================================\n\n");

    /* ========================================================
     * Phase 1: Interrupt subsystem
     * ======================================================== */
    /* Initialize Interrupt Controller */
    uart_printf("[BOOT] Initializing INTC...\n");
    intc_init();

    /* Initialize IRQ framework */
    uart_printf("[BOOT] Initializing IRQ framework...\n");
    irq_init();

    /* Initialize Hardware Timer */
    uart_printf("[BOOT] Initializing Timer...\n");
    timer_init();
    
    /* Initialize Scheduler */
    uart_printf("[BOOT] Initializing Scheduler...\n");
    scheduler_init();

    /* 
     * NOTE: Idle task is created automatically by scheduler_init()
     * We only need to add user/test tasks here.
     * UPDATE: Manually adding idle task for now to ensure explicit registration.
     */
    uart_printf("[BOOT] Creating idle task...\n");
    #include "idle.h" /* Typically at top, but allowed here for scope if needed, keeping simple */
    /* Better to put include at top, will fix in next step if strict C, but simpler here: */
    /* Wait, let's put include at top in separate edit or just implicit dec warning? */
    /* Actually, let's just add the code and assume header is clean or add include at top. */
    /* Strategy: I will add the include at the top in a separate edit to be clean. */
    /* Here I just add the logic. */
    
    struct task_struct *idle_ptr = get_idle_task();
    if (scheduler_add_task(idle_ptr) < 0) {
        uart_printf("[BOOT] ERROR: Failed to add idle task\n");
        while(1);
    }

    /* Create and add Test Task */
    uart_printf("[BOOT] Creating test task...\n");
    struct task_struct *t_task = create_test_task();
    
    uart_printf("[BOOT] Adding test task to scheduler...\n");
    if (scheduler_add_task(t_task) < 0) {
        uart_printf("[BOOT] ERROR: Failed to add test task\n");
        while(1);
    }

    uart_printf("[BOOT] Boot complete!\n");
    uart_printf("[BOOT] Total tasks: 2 (idle + test)\n");
    uart_printf("[BOOT] Time slice: 10ms per task\n");
    uart_printf("[BOOT] IRQs kept disabled (will enable on first task start)\n");
    uart_printf("[BOOT] Starting scheduler...\n");

    /* Start Scheduler (Never Returns) */
    scheduler_start();

    /* Should never reach here */
    uart_printf("[BOOT] FATAL: Returned from scheduler_start!\n");
    while (1);
}