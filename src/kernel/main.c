/* ============================================================
 * main.c
 * ------------------------------------------------------------
 * RefARM-OS: Interactive Shell
 * Target: BeagleBone Black (AM335x)
 * ============================================================ */

#include "uart.h"
#include "watchdog.h"
#include "intc.h"
#include "irq.h"
#include "cpu.h"
#include "shell.h"

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

    uart_printf("[BOOT] Enabling IRQ...\n");
    irq_enable();

    uart_printf("[BOOT] Done.\n\n");

    /* ========================================================
     * Phase 2: Shell
     * ======================================================== */
    shell_init();

    /* ========================================================
     * Phase 3: Main loop
     * ======================================================== */
    while (1) {
        int c = uart_getc();

        if (c >= 0) {
            shell_process_char((char)c);
        } else {
            wfi();
        }
    }
}