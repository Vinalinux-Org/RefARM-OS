/* ============================================================
 * kernel/main.c
 * ------------------------------------------------------------
 * Kernel entry point (minimal boot test)
 * Target: AM335x / BeagleBone Black
 * ============================================================
 */

#include <stdint.h>

/* ------------------------------------------------------------
 * Hardware base addresses
 * ------------------------------------------------------------
 */
#define WDT1_BASE       0x44E35000
#define CM_WKUP_BASE    0x44E00400
#define UART0_BASE      0x44E09000

/* ------------------------------------------------------------
 * Register offsets
 * ------------------------------------------------------------
 */
#define WDT_WSPR        0x48
#define WDT_WWPS        0x34
#define CM_WKUP_WDT1_CLKCTRL   0xD0

#define UART_THR        0x00
#define UART_LSR        0x14
#define UART_LSR_THRE   (1 << 5)

/* ------------------------------------------------------------
 * MMIO helpers
 * ------------------------------------------------------------
 */
static inline void writel(uint32_t val, uint32_t addr)
{
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t readl(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

/* ------------------------------------------------------------
 * Disable watchdog (mandatory or board will reset)
 * ------------------------------------------------------------
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

/* ------------------------------------------------------------
 * UART output (polling)
 * ------------------------------------------------------------
 */
static void putc(char c)
{
    while (!(readl(UART0_BASE + UART_LSR) & UART_LSR_THRE));
    writel(c, UART0_BASE + UART_THR);
}

static void puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            putc('\r');
        putc(*s++);
    }
}

/* ------------------------------------------------------------
 * Kernel entry point
 * ------------------------------------------------------------
 */
void kernel_main(void)
{
    disable_watchdog();

    puts("\n");
    puts("RefARM-OS\n");
    puts("Boot OK\n");

    /* Halt */
    while (1);
}