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
#include "mmu.h"
#include "cpu.h"
#include "vfs.h"
#include "ramfs.h"
#include "syscalls.h"
#include "types.h"
/* ============================================================
 * User Space Payload (Defined in payload.S)
 * ============================================================ */
extern uint8_t _shell_payload_start;
extern uint8_t _shell_payload_end;

/* ============================================================
 * User App Memory Definitions
 * ============================================================ */
static struct task_struct shell_task;

/* We use the end of the 1MB User Space (0x40000000 -> 0x40100000)
 * as the stack base for the User Task. Let's reserve the top 4KB. */
#define USER_STACK_BASE (USER_SPACE_VA + (USER_SPACE_MB * 1024 * 1024))
#define USER_STACK_SIZE 4096

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
    uart_printf(" VinixOS: Interactive Shell\n");
    uart_printf("========================================\n\n");

    /* 1.5 MMU Phase B — remove identity map, update VBAR to high VA.
     * MMU was already enabled by entry.S (Phase A trampoline).
     * We are now running at VA 0xC0xxxxxx. */
    mmu_init();

    intc_init();
    irq_init();
    uart_enable_rx_interrupt(); /* Enable UART RX interrupt for keyboard input */
    timer_init();

    /* 1.6 Initialize VFS and mount RAMFS */
    uart_printf("[BOOT] Initializing Virtual File System...\n");
    vfs_init();
    
    /* Initialize RAMFS */
    if (ramfs_init() != E_OK) {
        uart_printf("[BOOT] ERROR: Failed to initialize RAMFS\n");
        while (1);
    }
    
    /* Mount RAMFS at root */
    if (vfs_mount("/", ramfs_get_operations()) != E_OK) {
        uart_printf("[BOOT] ERROR: Failed to mount RAMFS at /\n");
        while (1);
    }

    /* 1.7 Load User Payload to 0x40000000 */
    uint32_t payload_size = (uint32_t)&_shell_payload_end - (uint32_t)&_shell_payload_start;
    uart_printf("[BOOT] Loading User App Payload to 0x%x (Size: %d bytes)\n", USER_SPACE_VA, payload_size);

    uint8_t *src = &_shell_payload_start;
    uint8_t *dst = (uint8_t *)USER_SPACE_VA;
    for (uint32_t i = 0; i < payload_size; i++)
    {
        dst[i] = src[i];
    }
    uart_printf("[BOOT] Payload successfully copied to User Space.\n");

    /* 2. Schedule Initialization */
    scheduler_init();

    /* 3. Add Idle Task (Kernel Mode) */
    struct task_struct *idle_ptr = get_idle_task();
    scheduler_add_task(idle_ptr);

    /* 4. Add User Application Task (Shell via SDK) */
    shell_task.name = "User App (Shell)";
    shell_task.state = TASK_STATE_READY;
    shell_task.id = 0; // Will be assigned 1

    /* True User App Entry = 0x40000000. Stack = End of 1MB User Space. */
    task_stack_init(&shell_task, (void (*)(void))USER_SPACE_VA,
                    (void *)(USER_STACK_BASE - USER_STACK_SIZE),
                    USER_STACK_SIZE);

    if (scheduler_add_task(&shell_task) < 0)
    {
        uart_printf("[BOOT] Failed to add User App Task\n");
    }

    uart_printf("[BOOT] Starting Scheduler...\n");

    /* Enable IRQ at CPU level (clear I-bit in CPSR).
     * Must be done AFTER all IRQ setup (intc, irq framework, handlers)
     * and BEFORE scheduler_start() so timer ticks and UART RX work. */
    irq_enable();

    scheduler_start();

    /* Should never reach here */
    while (1)
    {
        uart_printf("PANIC: Scheduler returned!\n");
    }
}