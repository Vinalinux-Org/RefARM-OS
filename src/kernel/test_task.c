/* ============================================================
 * test_task.c
 * ------------------------------------------------------------
 * Test task for RefARM-OS scheduler verification
 * Target: BeagleBone Black (ARMv7-A)
 * ============================================================ */

#include "task.h"
#include "uart.h"
#include "cpu.h"

/* Test task stack (1KB) */
#define TEST_STACK_SIZE 1024
static uint8_t test_stack[TEST_STACK_SIZE] __attribute__((aligned(8)));

/* Test task structure */
static struct task_struct test_task;

/**
 * Test task entry point
 * Independent counter to verify context switching
 */
static void test_task_func(void)
{
    uint32_t counter = 0;
    
    uart_printf("[TEST] Test task started\n");
    uart_printf("[TEST] Stack: 0x%08x - 0x%08x\n",
                (uint32_t)test_stack,
                (uint32_t)test_stack + TEST_STACK_SIZE);
    
    /* Infinite loop */
    while (1) {
        /* Increment counter */
        counter++;
        
        /* Log every 1000000 iterations */
        if (counter % 1000000 == 0) {
            uart_printf("[TEST] Running... counter=%u\n", counter);
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
    test_task.name = "test";
    test_task.state = TASK_STATE_READY;
    test_task.id = 0;  /* Will be set by scheduler */
    
    /* Initialize stack */
    task_stack_init(&test_task, test_task_func, test_stack, TEST_STACK_SIZE);
    
    return &test_task;
}
