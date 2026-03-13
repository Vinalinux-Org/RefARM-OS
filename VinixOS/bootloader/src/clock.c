/* ============================================================
 * clock.c
 * PLL and Clock Configuration for AM335x
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* ============================================================
 * Configure DDR PLL for 400 MHz output (DDR3 data pins)
 *
 * DDR PLL Configuration:
 * - Input clock: 24 MHz crystal oscillator
 * - Target output: 400 MHz for DDR3 data rate
 * - PLL formula: CLKOUT = [M / (N+1)] × CLKINP × [1/M2]
 *
 * Calculated parameters:
 *   M = 400  (multiplier)
 *   N = 23   (divider)
 *   M2 = 1   (post-divider)
 *
 * Verification:
 *   CLKOUT = [400/(23+1)] × 24 × [1/1] = 400 MHz ✓
 *   Internal clock (CLKDCOLDO) = 800 MHz (within 2000 MHz limit) ✓
 *   Reference clock = 24/(23+1) = 1 MHz (within 0.5-2.5 MHz range) ✓
 *
 * Clock domains:
 *   DDR PLL CLKOUT (400 MHz) → DDR data pins (DDR_CK, DQS, DQ)
 *   Core PLL M4 (200 MHz) → EMIF registers (m_clk, L3F_CLK)
 *
 * Register values:
 *   CM_CLKSEL_DPLL_DDR: M=400 (0x190 << 8), N=23 (0x17) → 0x19017
 *   CM_DIV_M2_DPLL_DDR: M2=1
 * ============================================================ */
static void config_ddr_pll(void)
{
    uint32_t clkmode, clksel, div_m2;

    /* Step 1: Put DDR PLL in MN bypass mode (required before changing M/N dividers)
     * This allows direct clock input while we reconfigure the PLL */
    clkmode = readl(CM_CLKMODE_DPLL_DDR);
    clkmode = (clkmode & ~0x7) | DPLL_MN_BYP_MODE;
    writel(clkmode, CM_CLKMODE_DPLL_DDR);

    /* Wait for bypass status (bit 8 = ST_MN_BYPASS)
     * Use timeout to avoid hang if PLL is stuck */
    for (int i = 0; i < 10000; i++) {
        if (readl(CM_IDLEST_DPLL_DDR) & 0x00000100) break;
    }

    /* Step 2: Program M and N dividers for 400 MHz output
     * M = 400 (multiplier), N = 23 (divider) */
    clksel = readl(CM_CLKSEL_DPLL_DDR);
    clksel &= ~0x7FFFF;           /* Clear bits [18:0] */
    clksel |= (400 << 8) | 23;   /* M=400 at bits [18:8], N=23 at bits [6:0] */
    writel(clksel, CM_CLKSEL_DPLL_DDR);

    /* Step 3: Set M2 post-divider = 1
     * Final output clock = 800MHz / 2 / 1 = 400 MHz */
    div_m2 = readl(CM_DIV_M2_DPLL_DDR);
    div_m2 = (div_m2 & ~0x1F) | 1;   /* Set bits [4:0] = 1 */
    writel(div_m2, CM_DIV_M2_DPLL_DDR);

    /* Step 4: Lock PLL - switch from bypass to lock mode */
    clkmode = readl(CM_CLKMODE_DPLL_DDR);
    clkmode = (clkmode & ~0x7) | DPLL_LOCK_MODE;
    writel(clkmode, CM_CLKMODE_DPLL_DDR);

    /* Step 5: Wait for PLL lock (bit 0 = ST_DPLL_CLK)
     * Use timeout to avoid infinite hang */
    for (int i = 0; i < 10000; i++) {
        if (readl(CM_IDLEST_DPLL_DDR) & 0x1) break;
    }
}


/* ============================================================
 * Enable peripheral clocks
 * ============================================================ */
static void enable_peripheral_clocks(void)
{
    /* Enable EMIF (DDR controller) clock */
    writel(MODULE_ENABLE, CM_PER_EMIF_CLKCTRL);
    while ((readl(CM_PER_EMIF_CLKCTRL) & 0x30000) != 0);

    /* Enable MMC0 clock */
    writel(MODULE_ENABLE, CM_PER_MMC0_CLKCTRL);
    while ((readl(CM_PER_MMC0_CLKCTRL) & 0x30000) != 0);
    
    /* Enable UART0 clock explicitly */
    writel(MODULE_ENABLE, CM_WKUP_UART0_CLKCTRL);
    while ((readl(CM_WKUP_UART0_CLKCTRL) & 0x30000) != 0);
}

/* ============================================================
 * Main clock initialization
 * ============================================================ */
void clock_init(void)
{
    /* BootROM has already configured:
     * - MPU PLL: 500 MHz (for CPU)
     * - PER PLL: 960 MHz (for peripherals, UART clock = 48 MHz)
     * - CORE PLL: 1000 MHz (for system)
     * We only need to configure DDR PLL for 400 MHz DDR3 operation.
     *
     * IMPORTANT: This function must not call UART functions.
     * UART logging is the caller's responsibility.
     */
    config_ddr_pll();

    /* Enable peripheral clocks (EMIF for DDR, MMC0 for SD card) */
    enable_peripheral_clocks();
}
