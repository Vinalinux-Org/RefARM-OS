/* ============================================================
 * clock.c
 * PLL and Clock Configuration
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* ============================================================
 * Configure DDR PLL (ADPLLS): 400 MHz output for DDR3 data pins
 *
 * TRM Ch8 sec 8.9.3 — ADPLLS Output Clock Frequencies formula:
 *   CLKOUT     = [M / (N+1)] × CLKINP × [1/M2]
 *   CLKDCOLDO  = 2 × [M / (N+1)] × CLKINP   (must be ≤ 2000 MHz)
 *
 * Crystal / CLKINP = 24 MHz (BBB: SYSBOOT[15:14] = 01b = 24MHz OSC)
 * Target DDR3 data rate = 400 MT/s → DDR clock = 400 MHz → CLKOUT = 400 MHz
 *
 * Solving:
 *   400 = [M / (23+1)] × 24 × [1/1]
 *   400 = [M / 24]     × 24
 *   400 = M
 *   → M = 400, N = 23, M2 = 1  ✓
 *
 * Verification:
 *   CLKOUT    = [400/24] × 24 / 1 = 400 MHz  → DDR data pin clock ✓
 *   CLKDCOLDO = 2 × [400/24] × 24 = 800 MHz  → ≤ 2000 MHz ✓
 *   REFCLK    = CLKINP / (N+1) = 24/24 = 1 MHz  ✓ (valid: REFCLK must be 0.5–2.5 MHz)
 *
 * IMPORTANT — DDR PLL M2 output (400 MHz) vs EMIF m_clk:
 *   DDR pin clocks  (DDR_CK, DQS, DQ) = DDR PLL CLKOUT = 400 MHz → to DDR chip
 *   EMIF OCP clock  (m_clk, L3F_CLK)  = Core PLL M4    = 200 MHz → to EMIF registers
 *   (TRM Ch8 sec 8.9.7: "L3F_CLK served to EMIF" = CORE_CLKOUTM4/M4=10 = 200 MHz)
 *
 * Register encoding:
 *   CM_CLKSEL_DPLL_DDR[18:8] = DPLL_MULT = M = 400 = 0x190
 *   CM_CLKSEL_DPLL_DDR[6:0]  = DPLL_DIV  = N = 23  = 0x17
 *   → write: (400 << 8) | 23 = 0x19017
 *   CM_DIV_M2_DPLL_DDR[4:0]  = DPLL_CLKOUT_DIV = M2 = 1
 * ============================================================ */
static void config_ddr_pll(void)
{
    uint32_t clkmode, clksel, div_m2;

    /* Step 1: Put DDR PLL in MN bypass mode (required before changing M/N)
     * TRM: DPLL_EN[2:0] = 0x4 = MN_BYP_MODE */
    clkmode = readl(CM_CLKMODE_DPLL_DDR);
    clkmode = (clkmode & ~0x7) | DPLL_MN_BYP_MODE;
    writel(clkmode, CM_CLKMODE_DPLL_DDR);

    /* Wait for bypass status: CM_IDLEST_DPLL_DDR.ST_MN_BYPASS (bit 8) = 1
     * Use timeout to avoid hang if PLL is stuck */
    for (int i = 0; i < 10000; i++) {
        if (readl(CM_IDLEST_DPLL_DDR) & 0x00000100) break;
    }

    /* Step 2: Program M and N dividers
     * CM_CLKSEL_DPLL_DDR[18:8] = M = 400
     * CM_CLKSEL_DPLL_DDR[6:0]  = N = 23
     * Result: CLKOUT = [400/(23+1)] × 24MHz × [1/M2] = 400 MHz */
    clksel = readl(CM_CLKSEL_DPLL_DDR);
    clksel &= ~0x7FFFF;           /* Clear bits [18:0] */
    clksel |= (400 << 8) | 23;   /* M=400 at [18:8], N=23 at [6:0] */
    writel(clksel, CM_CLKSEL_DPLL_DDR);

    /* Step 3: Set M2 post-divider = 1
     * CLKOUT = CLKDCOLDO / 2 / M2 = 800MHz / 2 / 1 = 400 MHz
     * This clock drives DDR data pin macros via DDR PLL CLKOUT path */
    div_m2 = readl(CM_DIV_M2_DPLL_DDR);
    div_m2 = (div_m2 & ~0x1F) | 1;   /* CM_DIV_M2_DPLL_DDR[4:0] = 1 */
    writel(div_m2, CM_DIV_M2_DPLL_DDR);

    /* Step 4: Lock PLL — DPLL_EN[2:0] = 0x7 = LOCK mode */
    clkmode = readl(CM_CLKMODE_DPLL_DDR);
    clkmode = (clkmode & ~0x7) | DPLL_LOCK_MODE;
    writel(clkmode, CM_CLKMODE_DPLL_DDR);

    /* Step 5: Wait for lock — CM_IDLEST_DPLL_DDR.ST_DPLL_CLK (bit 0) = 1
     * Use timeout to avoid infinite hang on silicon/hardware issue */
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
    /* According to AM335x BootROM documentation:
     * ROM Code already configures:
     *   - MPU PLL: 500 MHz (locked)
     *   - PER PLL: 960 MHz (locked), UART clock = 48 MHz from PER_CLKOUTM2
     *   - CORE PLL: 1000 MHz (locked)
     * We only NEED to configure DDR PLL for 400 MHz DDR3 operation.
     *
     * IMPORTANT: This function must NOT call uart_init() or uart_puts().
     * UART logging is the responsibility of the caller (bootloader_main).
     * Calling uart_init() here disrupts in-flight UART TX and causes
     * garbled output. PER PLL (UART clock source) is untouched anyway.
     */
    config_ddr_pll();

    /* Enable peripheral clocks (EMIF for DDR, MMC0 for SD card) */
    enable_peripheral_clocks();
}
