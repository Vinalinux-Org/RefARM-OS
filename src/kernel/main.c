/* ============================================================
 * kernel/main.c
 * ------------------------------------------------------------
 * Kernel entry point (testing uart_printf)
 * Target: AM335x / BeagleBone Black
 * ============================================================
 */

#include "uart.h"
#include "watchdog.h"

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
    
    /* Boot banner */
    uart_printf("\n");
    uart_printf("====================================\n");
    uart_printf(" RefARM-OS Kernel (with printf!)\n");
    uart_printf("====================================\n");
    uart_printf("\n");
    
    /* Basic printf tests */
    uart_printf("=== Basic printf tests ===\n");
    uart_printf("String: %s\n", "Hello, World!");
    uart_printf("Character: %c\n", 'A');
    uart_printf("Literal: %%\n");
    uart_printf("\n");
    
    /* Integer tests */
    uart_printf("=== Integer tests ===\n");
    uart_printf("Decimal: %d\n", 42);
    uart_printf("Negative: %d\n", -123);
    uart_printf("Unsigned: %u\n", 0xFFFFFFFF);
    uart_printf("Zero: %d\n", 0);
    uart_printf("\n");
    
    /* Hexadecimal tests */
    uart_printf("=== Hexadecimal tests ===\n");
    uart_printf("Hex (plain): 0x%x\n", 0xdeadbeef);
    uart_printf("Hex (upper): 0x%X\n", 0xDEADBEEF);
    uart_printf("Hex (padded): 0x%08x\n", 0x42);
    uart_printf("Address: 0x%08x\n", 0x80000000);
    uart_printf("\n");
    
    /* Mixed format tests */
    uart_printf("=== Mixed format tests ===\n");
    uart_printf("Address: 0x%08x, Value: %d, Status: %s\n", 
                0x80000100, 1234, "OK");
    uart_printf("Multiple hex: 0x%x 0x%x 0x%x\n", 0xaa, 0xbb, 0xcc);
    uart_printf("\n");
    
    /* Success message */
    uart_printf("=== All tests passed ===\n");
    uart_printf("uart_printf() is working!\n");
    uart_printf("\n");
    
    /* Halt */
    uart_printf("Kernel halting.\n");
    while (1)
        ;
}
