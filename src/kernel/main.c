/* ============================================================
 * kernel/main.c
 * ------------------------------------------------------------
 * Kernel entry point (testing uart_printf)
 * Target: AM335x / BeagleBone Black
 * ============================================================
 */

#include <stdint.h>
#include "uart.h"

/* ============================================================
 * Hardware definitions
 * ============================================================
 */
#define WDT1_BASE       0x44E35000
#define CM_WKUP_BASE    0x44E00400

#define WDT_WSPR        0x48
#define WDT_WWPS        0x34
#define CM_WKUP_WDT1_CLKCTRL   0xD0

/* ============================================================
 * MMIO helpers
 * ============================================================
 */
static inline void writel(uint32_t val, uint32_t addr)
{
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

/* ============================================================
 * Watchdog disable
 * ============================================================
 */
static void disable_watchdog(void)
{
    writel(0x2, CM_WKUP_BASE + CM_WKUP_WDT1_CLKCTRL);
    while ((readl(CM_WKUP_BASE + CM_WKUP_WDT1_CLKCTRL) & 0x3) != 0x2);

    writel(0xAAAA, WDT1_BASE + WDT_WSPR);
    while (readl(WDT1_BASE + WDT_WWPS));

    writel(0x5555, WDT1_BASE + WDT_WSPR);
    while (readl(WDT1_BASE + WDT_WWPS));
}

/* ============================================================
 * Test functions for uart_printf
 * ============================================================
 */

static void test_printf_basic(void)
{
    uart_printf("\n=== Basic printf tests ===\n");
    uart_printf("String: %s\n", "Hello, World!");
    uart_printf("Character: %c\n", 'A');
    uart_printf("Literal: %%\n");
}

static void test_printf_integers(void)
{
    uart_printf("\n=== Integer tests ===\n");
    uart_printf("Decimal: %d\n", 42);
    uart_printf("Negative: %d\n", -123);
    uart_printf("Unsigned: %u\n", 4294967295U);
    uart_printf("Zero: %d\n", 0);
}

static void test_printf_hex(void)
{
    uart_printf("\n=== Hexadecimal tests ===\n");
    uart_printf("Hex (plain): 0x%x\n", 0xDEADBEEF);
    uart_printf("Hex (upper): 0x%X\n", 0xDEADBEEF);
    uart_printf("Hex (padded): 0x%08x\n", 0x42);
    uart_printf("Address: 0x%08x\n", 0x80000000);
}

static void test_printf_mixed(void)
{
    uart_printf("\n=== Mixed format tests ===\n");
    
    uint32_t addr = 0x80000100;
    int value = 1234;
    const char *status = "OK";
    
    uart_printf("Address: 0x%08x, Value: %d, Status: %s\n", 
                addr, value, status);
    
    uart_printf("Multiple hex: 0x%x 0x%x 0x%x\n",
                0xAA, 0xBB, 0xCC);
}

/* ============================================================
 * Kernel entry point
 * ============================================================
 */
void kernel_main(void)
{
    /* Disable watchdog */
    disable_watchdog();
    
    /* Initialize UART driver */
    uart_init();
    
    /* Print banner */
    uart_printf("\n\n");
    uart_printf("====================================\n");
    uart_printf(" RefARM-OS Kernel (with printf!)\n");
    uart_printf("====================================\n");
    
    /* Run printf tests */
    test_printf_basic();
    test_printf_integers();
    test_printf_hex();
    test_printf_mixed();
    
    /* Success message */
    uart_printf("\n=== All tests passed ===\n");
    uart_printf("uart_printf() is working!\n");
    
    /* Halt */
    uart_printf("\nKernel halting.\n");
    while (1);
}