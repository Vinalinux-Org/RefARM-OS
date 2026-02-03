/* ============================================================
 * test_task.c
 * ------------------------------------------------------------
 * Test task for RefARM-OS scheduler verification
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "test_task.h"
#include "task.h"
#include "uart.h"
#include <stdbool.h>
#include "cpu.h"

/* Test task stack (1KB) */
#define TEST_STACK_SIZE 1024
#define PRINT_INTERVAL 1000000
static uint8_t test_stack[TEST_STACK_SIZE] __attribute__((aligned(8)));

/* Test task structure */
static struct task_struct test_task_struct;

/**
 * Test task entry point
 * Independent counter to verify context switching
 */
static void test_task(void)
{
    uint32_t counter = 0;
    
    uart_printf("[TEST] Test task started\n");
    uart_printf("[TEST] Stack: 0x%08x - 0x%08x\n",
                (uint32_t)&test_stack[0],
                (uint32_t)&test_stack[TEST_STACK_SIZE]);
    
    /* Main test loop */
    while (1) {
        counter++;
        
        /* Print status periodically */
        if (counter % PRINT_INTERVAL == 0) {
            uart_printf("[TEST] Running... counter=%u\n", counter);
        }
        
        /* 
         * Check if scheduler wants us to yield
         * This is the cooperative yield point
         */
        extern volatile bool need_reschedule;
        if (need_reschedule) {
            extern void scheduler_yield(void);
            scheduler_yield();
            /* ← Task resumes here after being switched back */
        }
    }
}

/**
 * Get test task structure
 * @return Pointer to test task
 */
struct task_struct *get_test_task(void)
{
    /* Initialize test task */
    test_task_struct.name = "test";
    test_task_struct.state = TASK_STATE_READY;
    test_task_struct.id = 0;  /* Will be set by scheduler */
    
    /* Initialize stack */
    task_stack_init(&test_task_struct, test_task, test_stack, TEST_STACK_SIZE);
    
    return &test_task_struct;
}
