/* ============================================================
 * kernel/main.c
 * ------------------------------------------------------------
 * Kernel entry point with IRQ infrastructure
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#include "uart.h"
#include "watchdog.h"
#include "exception.h"
#include "irq.h"
#include "intc.h"
#include "timer.h"
#include "cpu.h"

/* ============================================================
 * Main Kernel Entry Point
 * ============================================================ */

void kernel_main(void)
{
    /*
     * CRITICAL: Disable watchdog FIRST
     * AM335x boots with watchdog enabled (~3 minute timeout)
     * Must disable before any other operations
     */
    watchdog_disable();
    
    /* Initialize UART for debug output */
    uart_init();
    
    /* ========================================
     * Kernel Startup Banner
     * ======================================== */
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" RefARM-OS Kernel Starting\n");
    uart_printf(" Target: AM335x BeagleBone Black\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    
    /* ========================================
     * Initialize Interrupt Infrastructure
     * ======================================== */
    
    uart_printf("Initializing INTC...\n");
    intc_init();
    
    uart_printf("Initializing IRQ framework...\n");
    irq_init();
    
    /* ========================================
     * Initialize Peripheral Drivers
     * ======================================== */
    
    uart_printf("Initializing Timer2 (test driver)...\n");
    timer_init();
    
    /* ========================================
     * Enable IRQ in CPSR
     * ======================================== */
    
    uart_printf("\n");
    uart_printf("Enabling IRQ in CPSR...\n");
    irq_enable();
    
    /* Verify IRQ is enabled */
    if (irq_is_enabled()) {
        uart_printf("IRQ successfully enabled\n");
    } else {
        uart_printf("ERROR: IRQ not enabled!\n");
    }
    
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" Kernel Ready\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("System is now running with interrupts enabled.\n");
    uart_printf("Timer2 will generate periodic interrupts.\n");
    uart_printf("Press Ctrl+C to exit serial console.\n");
    uart_printf("\n");
    
    /* ========================================
     * Main Loop
     * ======================================== */
    
    /*
     * Idle loop with WFI (Wait For Interrupt)
     * 
     * WFI puts CPU into low-power state until interrupt occurs.
     * When interrupt fires:
     * 1. CPU wakes up
     * 2. IRQ handler executes
     * 3. Return to WFI
     * 
     * This is the standard idle loop for embedded systems.
     */
    while (1) {
        wfi();  /* Wait for interrupt */
    }
}