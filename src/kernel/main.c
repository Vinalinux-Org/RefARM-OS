/* ============================================================
 * main.c
 * ------------------------------------------------------------
 * RefARM-OS: Phase 7 - Two Task Context Switch Test
 * Target: BeagleBone Black (AM335x)
 * ============================================================ */

#include "uart.h"
#include "watchdog.h"
#include "intc.h"
#include "irq.h"
#include "cpu.h"
#include "timer.h"
#include "scheduler.h"
#include "idle.h"
#include "test_task.h"

void kernel_main(void)
{
    struct task_struct *idle;
    struct task_struct *test;
    
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
    uart_printf("[BOOT] Initializing INTC...\n");
    intc_init();

    uart_printf("[BOOT] Initializing IRQ framework...\n");
    irq_init();

    uart_printf("[BOOT] Enabling UART RX interrupt...\n");
    uart_enable_rx_interrupt();

    uart_printf("[BOOT] Done.\n\n");

    /* ========================================================
     * Phase 2: Timer (for scheduler ticks)
     * ======================================================== */
    uart_printf("[BOOT] Initializing Timer...\n");
    timer_init();
    uart_printf("[BOOT] Timer configured for 10ms ticks (100 Hz)\n\n");

    /* ========================================================
     * Phase 3: Scheduler
     * ======================================================== */
    uart_printf("[BOOT] Initializing Scheduler...\n");
    scheduler_init();
    
    uart_printf("[BOOT] Creating idle task...\n");
    idle = get_idle_task();
    
    uart_printf("[BOOT] Adding idle task to scheduler...\n");
    scheduler_add_task(idle);
    
    uart_printf("\n[BOOT] Creating test task...\n");
    test = get_test_task();
    
    uart_printf("[BOOT] Adding test task to scheduler...\n");
    scheduler_add_task(test);
    
    uart_printf("\n[BOOT] Boot complete!\n");
    uart_printf("[BOOT] Total tasks: 2 (idle + test)\n");
    uart_printf("[BOOT] Time slice: 10ms per task\n");
    uart_printf("[BOOT] IRQs kept disabled (will enable on first task start)\n");
    // irq_enable();  <-- REMOVED to fix race condition
    
    /* ========================================================
     * Phase 4: Start Scheduler (NEVER RETURNS)
     * ======================================================== */
    uart_printf("[BOOT] Starting scheduler...\n\n");
    scheduler_start();
    
    /* Should never reach here */
    uart_printf("[BOOT] FATAL: Returned from scheduler_start!\n");
    while (1);
}