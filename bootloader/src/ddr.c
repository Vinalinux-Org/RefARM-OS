/* ============================================================
 * ddr.c
 * DDR3 Memory Initialization (RefARM-OS Custom Implementation)
 * 
 * Target: MT41K256M16HA-125 (256MB DDR3, 16-bit width)
 * Frequency: 400 MHz (EMIF 200 MHz)
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* ============================================================
 * DDR3 Timing Parameters for MT41K256M16HA-125 @ 400MHz
 *
 * Reference: U-Boot am335x_evm DDR3 configuration (verified working on BBB)
 *            MT41K256M16HA-125 datasheet (Micron)
 *            AM335x TRM Chapter 7 (EMIF4F)
 *
 * !! CRITICAL CLOCK DOMAIN NOTE !!
 *   SDRAM_TIM_1/2/3 register fields use DDR clock cycles (tCK = 2.5ns @400MHz)
 *   NOT EMIF OCP clock periods (5ns @200MHz).
 *   Formula: field_value = ceil(time_ns / 2.5) - 1  (stored as N-1)
 *
 * Clock Domains:
 *   DDR PLL CLKOUT  (data/cmd pins)  = 400 MHz → tCK = 2.5 ns (timing base!)
 *   EMIF m_clk      (OCP interface)  = 200 MHz → 5.0 ns (register access only)
 * ============================================================ */

/* SDRAM_TIM_1 (EMIF offset 0x18) - U-Boot reference: 0x0AAAE51B
 * All fields in DDR clock cycles (tCK = 2.5ns) minus 1:
 *   [28:25] T_RP   = 5  → 6 × 2.5ns = 15.0ns  ≥ tRP_min  = 13.75ns ✓
 *   [24:21] T_RCD  = 5  → 6 × 2.5ns = 15.0ns  ≥ tRCD_min = 13.75ns ✓
 *   [20:17] T_WR   = 5  → 6 × 2.5ns = 15.0ns  ≥ tWR_min  = 15.0ns  ✓
 *   [16:12] T_RAS  = 14 → 15 × 2.5ns = 37.5ns ≥ tRAS_min = 35.0ns  ✓
 *   [11:6]  T_RC   = 20 → 21 × 2.5ns = 52.5ns ≥ tRC_min  = 48.75ns ✓
 *   [5:3]   T_RRD  = 3  → 4 × 2.5ns = 10.0ns  ≥ tRRD_min = 7.5ns   ✓
 *   [2:0]   T_WTR  = 3  → 4 × 2.5ns = 10.0ns  ≥ tWTR_min = 7.5ns   ✓ */
#define EMIF_TIM1   0x0AAAE51B

/* SDRAM_TIM_2 (EMIF offset 0x20) - U-Boot reference: 0x266B7FDA
 *   [30:28] T_XP   = 2  → 3 tCK = 7.5ns   ≥ tXP_min  = 6.0ns  ✓
 *   [24:16] T_XSNR = 0x6B = 107 cycles     → 107×2.5 = 267.5ns ≥ tXSNR = 170ns ✓
 *   [15:6]  T_XSRD = 0x1FF = 511 cycles    → 512 tCK (self-refresh exit) ✓
 *   [5:3]   T_RTP  = 3  → 4 tCK = 10.0ns  ≥ tRTP_min = 7.5ns  ✓
 *   [2:0]   T_CKE  = 2  → 3 tCK = 7.5ns   ≥ tCKE_min = 5.0ns  ✓ */
#define EMIF_TIM2   0x266B7FDA

/* SDRAM_TIM_3 (EMIF offset 0x28) - U-Boot reference: 0x501F867F
 *   [31:28] T_PDLL_UL = 5   (DLL unlock time, safe for DDR3)
 *   [20:15] ZQ_ZQCS   = 0x3F = 63 → 64 tCK for ZQCS calibration ✓
 *   [12:4]  T_RFC     = 0x67 = 103 → 104×2.5ns = 260ns ≥ tRFC=160ns ✓ (conservative)
 *   [3:0]   T_RAS_MAX = 0xF → max, refresh interval handles compliance ✓ */
#define EMIF_TIM3   0x501F867F

/* SDRAM_REF_CTRL (EMIF offset 0x10) - U-Boot reference: 0x00000C30
 * REFRESH_RATE[15:0] = 0x0C30 = 3120 DDR clock cycles
 * 3120 × 2.5ns = 7800ns = 7.8µs ← matches DDR3 tREFI spec exactly ✓ */
#define EMIF_REF_CTRL 0x00000C30

/* EMIF SDRAM Configuration - U-Boot reference: 0x61C05332
 *
 * Bit-by-bit decode of 0x61C05332:
 *   0x61C05332 = 0110_0001_1100_0000_0101_0011_0011_0010
 *
 *   [31:29] SDRAM_TYPE  = 011   → DDR3 ✓
 *   [28:27] IBANK_POS   = 00    → bank addr before column ✓
 *   [26:24] DDR_TERM    = 001   → RZQ/4 (ODT) ✓
 *   [23:18] reserved    = 110000→ (TI silicon defaults, do not touch)
 *   [17:16] CWL         = 00    → CAS Write Latency = 5 (DDR3 encoding: 0=5) ✓
 *   [15:14] NARROW_MODE = 01    → !! 16-bit bus !! (MANDATORY for AM335x) ✓
 *   [13:10] CL          = 0100  → CAS Latency 6 (DDR3 encoding: 4=CL6) ✓
 *   [9:7]   ROWSIZE     = 110   → 15-bit row address (32K rows for 1Gbit) ✓
 *   [6:4]   IBANK       = 011   → 8 internal banks ✓
 *   [2:0]   PAGESIZE    = 010   → 10-bit column (1024 columns) ✓ */
#define EMIF_CFG    0x61C05332

/* DDR PHY Control - U-Boot reference: 0x00100007
 * [20]  PHY_ENABLE_DYNAMIC_PWRDN = 1 (saves power between bursts)
 * [4:0] READ_LATENCY = 7 (CL=6 + 1 pipeline stage) ✓ */
#define DDR_PHY_CTRL    0x00100007

/* IO Impedance Control for DDR pads
 * 0x18B: reference value from AM335x DDR3 HW Design Guide (SPRABY0) */
#define DDR_IO_CTRL_VAL 0x0000018B

/* VTP bit definitions (Control Module: 0x44E10E0C) */
#define VTP_CTRL_ENABLE   (1 << 6)  /* bit 6: CLRZ — 0=reset, 1=enable compensation */
#define VTP_CTRL_READY    (1 << 5)  /* bit 5: READY (read-only, set when calibrated) */

/* ============================================================
 * DDR3 Initialization Sequence
 * ============================================================ */

void ddr_init(void)
{
    uint32_t i;
    uart_putc('6'); /* CHECKPOINT 6: Start DDR Init */

    /* --------------------------------------------------------
     * Step 1: Enable VTP (Voltage Temperature Impedance Compensation)
     * Sequence per AM335x HW Design Guide (SPRABY0):
     * 1. Set CLRZ=0 to force reset (bit 6 active-low)
     * 2. Set CLRZ=1 to release and start VTP calibration
     * 3. Wait for READY bit (bit 5) to assert
     * -------------------------------------------------------- */
    /* Step 1a: Force VTP reset (CLRZ=0) */
    writel(readl(VTP_CTRL) & ~VTP_CTRL_ENABLE, VTP_CTRL);

    /* Step 1b: Enable VTP by asserting CLRZ=1 (bit 6) */
    writel(readl(VTP_CTRL) | VTP_CTRL_ENABLE, VTP_CTRL);

    /* Step 1c: Wait for VTP READY (bit 5) with timeout.
     * READY asserts when internal calibration converges. */
    for (i = 0; i < 10000; i++) {
        if (readl(VTP_CTRL) & VTP_CTRL_READY) break;
    }
    uart_putc('v'); /* CHECKPOINT v: VTP Calibration Done */

    uart_putc('7'); /* CHECKPOINT 7: VTP Loop Done */

    /* --------------------------------------------------------
     * Step 2: Configure DDR IO Control
     * Control Module pads for DDR3 signal integrity.
     * Must set ALL cmd and data pad groups (TRM Ch9).
     * -------------------------------------------------------- */
    writel(DDR_IO_CTRL_VAL, DDR_IO_CTRL);        /* DDR CMD pads */
    writel(DDR_IO_CTRL_VAL, DDR_DATA0_IOCTRL);   /* DDR DATA byte 0 pads */
    writel(DDR_IO_CTRL_VAL, DDR_DATA1_IOCTRL);   /* DDR DATA byte 1 pads */

    /* Enable CKE Control (Manual) per Reference */
    writel(0x1, DDR_CKE_CTRL);

    /* --------------------------------------------------------
     * Step 3: DDR PHY Init (Slave Ratios) - CRITICAL for 400MHz
     * -------------------------------------------------------- */
    /* Command Macro Ratios */
    writel(0x80, DDR_CMD0_SLAVE_RATIO_0);
    writel(0x80, DDR_CMD1_SLAVE_RATIO_0);
    writel(0x80, DDR_CMD2_SLAVE_RATIO_0);
    
    writel(0x00, DDR_CMD0_INVERT_CLKOUT_0);
    writel(0x00, DDR_CMD1_INVERT_CLKOUT_0);
    writel(0x00, DDR_CMD2_INVERT_CLKOUT_0);

    /* DATA0 Ratios (Byte 0) */
    /* DATA0 Ratios (Byte 0) */
    writel(0x38, DDR_DATA0_RD_DQS_SLAVE_RATIO_0);
    writel(0x44, DDR_DATA0_WR_DQS_SLAVE_RATIO_0);
    writel(0x94, DDR_DATA0_FIFO_WE_SLAVE_RATIO_0);
    writel(0x7D, DDR_DATA0_WR_DATA_SLAVE_RATIO_0); // Reference uses 0x7D
    writel(0x00, DDR_DATA0_GATE_LEVEL_INIT_RATIO_0); 

    /* DATA1 Ratios (Byte 1) */
    writel(0x38, DDR_DATA1_RD_DQS_SLAVE_RATIO_0);
    writel(0x44, DDR_DATA1_WR_DQS_SLAVE_RATIO_0);
    writel(0x94, DDR_DATA1_FIFO_WE_SLAVE_RATIO_0);
    writel(0x7D, DDR_DATA1_WR_DATA_SLAVE_RATIO_0); // Reference uses 0x7D
    writel(0x00, DDR_DATA1_GATE_LEVEL_INIT_RATIO_0);

    /* Set PHY_CTRL_1 to trigger initialization? 
     * Usually done via EMIF configuration later. 
     */

    /* --------------------------------------------------------
     * Step 5: Configure EMIF
     * -------------------------------------------------------- */
    
    /* Set DDR PHY control */
    writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_1);
    writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_2);

    /* Set timing parameters (calculated from MT41K256M16HA-125 datasheet) */
    writel(EMIF_TIM1, EMIF_SDRAM_TIM_1);
    writel(EMIF_TIM2, EMIF_SDRAM_TIM_2);
    writel(EMIF_TIM3, EMIF_SDRAM_TIM_3);

    /* Set refresh rate: 7.8us = 1560 EMIF cycles @ 200MHz.
     * INITREF_DIS=0 → EMIF will auto-initialize SDRAM on first write. */
    writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);

    /* Configure SDRAM: DDR3, 16-bit, CL=6, CWL=5, 8-bank, 10-bit col */
    writel(EMIF_CFG, EMIF_SDRAM_CONFIG);

    /* --------------------------------------------------------
     * Step 6: Wait for DDR initialization to complete
     * EMIF auto-runs DDR3 init sequence after SDRAM_CONFIG write.
     * Delay ~500us to allow ZQCL and mode register loads to finish.
     * -------------------------------------------------------- */
    for (i = 0; i < 5000; i++);  /* ~500us at 200MHz */

    uart_putc('9'); /* CHECKPOINT 9: DDR Config OK */

    /* Re-write REF_CTRL after init to ensure refresh is active
     * (EMIF may disable refresh during initialization phase). */
    writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);

    /* Note: Static PHY slave ratios may not work for all boards.
     * If ddr_test() fails, software leveling is required:
     * EMIF_READ_WRITE_LEVELING_CONTROL = 0x80000000 (trigger) */
}



/* ============================================================
 * Simple memory test
 * ============================================================ */
int ddr_test(void)
{
    volatile uint32_t *ddr = (volatile uint32_t *)DDR_BASE;
    uint32_t pattern1 = 0xAA55AA55;
    uint32_t pattern2 = 0x55AA55AA;
    uint32_t read_val;
    int i;
    
    uart_putc('T'); /* Start Test */

    /* Write pattern 1 */
    for (i = 0; i < 1024; i++) {
        ddr[i] = pattern1;
    }

    /* Verify pattern 1 */
    for (i = 0; i < 1024; i++) {
        read_val = ddr[i];
        if (read_val != pattern1) {
            uart_puts("\r\nFAIL P1 @ ");
            uart_print_hex((uint32_t)&ddr[i]);
            uart_puts(": R=");
            uart_print_hex(read_val);
            uart_puts(" E=");
            uart_print_hex(pattern1);
            return -1;  /* Test failed */
        }
    }
    
    uart_putc('.');

    /* Write pattern 2 */
    for (i = 0; i < 1024; i++) {
        ddr[i] = pattern2;
    }

    /* Verify pattern 2 */
    for (i = 0; i < 1024; i++) {
        read_val = ddr[i];
        if (read_val != pattern2) {
            uart_puts("\r\nFAIL P2 @ ");
            uart_print_hex((uint32_t)&ddr[i]);
            uart_puts(": R=");
            uart_print_hex(read_val);
            return -1;  /* Test failed */
        }
    }
    
    uart_putc('K');
    return 0;  /* Test passed */
}