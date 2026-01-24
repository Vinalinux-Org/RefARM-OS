/* ============================================================
 * kernel/main.c
 * ------------------------------------------------------------
 * Kernel entry point after boot (called from entry.S)
 * Target: AM335x / Cortex-A8
 * ============================================================
 */

#include <stdint.h>

/* ------------------------------------------------------------
 * Hardware base addresses (from memory_map.md)
 * ------------------------------------------------------------
 */
#define WDT1_BASE       0x44E35000
#define CM_PER_BASE     0x44E00000
#define CM_WKUP_BASE    0x44E00400
#define UART0_BASE      0x44E09000

/* ------------------------------------------------------------
 * Watchdog registers (WDT1)
 * ------------------------------------------------------------
 */
#define WDT_WSPR        0x48
#define WDT_WWPS        0x34

/* ------------------------------------------------------------
 * Clock Module registers
 * ------------------------------------------------------------
 */
#define CM_PER_UART0_CLKCTRL   0x6C
#define CM_WKUP_WDT1_CLKCTRL   0xD0

/* ------------------------------------------------------------
 * UART registers
 * ------------------------------------------------------------
 */
#define UART_THR        0x00
#define UART_LSR        0x14
#define UART_LSR_THRE   (1 << 5)

/* ------------------------------------------------------------
 * MMIO helpers
 * ------------------------------------------------------------
 */
static inline void mmio_write(uint32_t addr, uint32_t value)
{
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t mmio_read(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

/* ------------------------------------------------------------
 * Busy wait
 * ------------------------------------------------------------
 */
static void delay(volatile uint32_t count)
{
    while (count--) {
        __asm__ volatile ("nop");
    }
}

/* ------------------------------------------------------------
 * Disable Watchdog (WDT1)
 * ------------------------------------------------------------
 * Mandatory on AM335x or board will reset.
 */
static void disable_watchdog(void)
{
    /* Enable clock for WDT1 */
    mmio_write(CM_WKUP_BASE + CM_WKUP_WDT1_CLKCTRL, 0x2);
    while ((mmio_read(CM_WKUP_BASE + CM_WKUP_WDT1_CLKCTRL) & 0x3) != 0x2);

    /* Disable sequence */
    mmio_write(WDT1_BASE + WDT_WSPR, 0xAAAA);
    while (mmio_read(WDT1_BASE + WDT_WWPS));

    mmio_write(WDT1_BASE + WDT_WSPR, 0x5555);
    while (mmio_read(WDT1_BASE + WDT_WWPS));
}

/* ------------------------------------------------------------
 * Enable UART0 clock
 * ------------------------------------------------------------
 */
static void enable_uart0_clock(void)
{
    mmio_write(CM_PER_BASE + CM_PER_UART0_CLKCTRL, 0x2);
    while ((mmio_read(CM_PER_BASE + CM_PER_UART0_CLKCTRL) & 0x3) != 0x2);
}

/* ------------------------------------------------------------
 * UART output (polling)
 * ------------------------------------------------------------
 */
static void uart_putc(char c)
{
    /* Wait for TX FIFO empty */
    while (!(mmio_read(UART0_BASE + UART_LSR) & UART_LSR_THRE));
    mmio_write(UART0_BASE + UART_THR, c);
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/* ------------------------------------------------------------
 * Kernel entry point
 * ------------------------------------------------------------
 */
void kernel_main(void)
{
    disable_watchdog();
    enable_uart0_clock();

    delay(10000);

    uart_puts("\n\n");
    uart_puts("====================================\n");
    uart_puts(" RefARM-OS Kernel Started (BBB)\n");
    uart_puts("====================================\n");

    uart_puts("Kernel is alive.\n");

    /* --------------------------------------------------------
     * Halt here for now
     * --------------------------------------------------------
     */
    while (1) {
        delay(1000000);
        uart_puts(".");
    }
}
