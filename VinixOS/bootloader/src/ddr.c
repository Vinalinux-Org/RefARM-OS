/* ============================================================
 * ddr.c
 * DDR3 Memory Initialization for AM335x
 * 
 * Target: MT41K256M16HA-125 (256MB DDR3, 16-bit width)
 * Frequency: 400 MHz (DDR data pins), 200 MHz (EMIF interface)
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* ============================================================
 * DDR3 Timing Parameters for 400MHz operation
 *
 * Timing values based on MT41K256M16HA-125 DDR3 chip specifications
 * All timing fields use DDR clock cycles (tCK = 2.5ns @ 400MHz)
 * Formula: field_value = ceil(time_ns / 2.5) - 1
 *
 * Clock domains:
 *   DDR PLL CLKOUT: 400 MHz → DDR data/command pins (tCK = 2.5ns)
 *   EMIF m_clk: 200 MHz → EMIF register interface (5.0ns)
 * ============================================================ */

/* SDRAM_TIM_1 (EMIF offset 0x18) - Timing parameters 1
 * Values provide safe margins above DDR3 minimum requirements:
 *   T_RP:   15.0ns (≥ 13.75ns min)
 *   T_RCD:  15.0ns (≥ 13.75ns min)  
 *   T_WR:   15.0ns (≥ 15.0ns min)
 *   T_RAS:  37.5ns (≥ 35.0ns min)
 *   T_RC:   52.5ns (≥ 48.75ns min)
 *   T_RRD:  10.0ns (≥ 7.5ns min)
 *   T_WTR:  10.0ns (≥ 7.5ns min) */
#define EMIF_TIM1   0x0AAAE51B

/* SDRAM_TIM_2 (EMIF offset 0x20) - Timing parameters 2
 *   T_XP:    7.5ns (≥ 6.0ns min)
 *   T_XSNR: 267.5ns (≥ 170ns min)
 *   T_XSRD: 512 tCK (self-refresh exit)
 *   T_RTP:  10.0ns (≥ 7.5ns min)
 *   T_CKE:   7.5ns (≥ 5.0ns min) */
#define EMIF_TIM2   0x266B7FDA

/* SDRAM_TIM_3 (EMIF offset 0x28) - Timing parameters 3
 *   T_PDLL_UL: 5 (DLL unlock time for DDR3)
 *   ZQ_ZQCS:   64 tCK for ZQ calibration
 *   T_RFC:    260ns (≥ 160ns min, conservative)
 *   T_RAS_MAX: maximum value */
#define EMIF_TIM3   0x501F867F

/* SDRAM_REF_CTRL (EMIF offset 0x10) - Refresh control
 * Refresh rate: 3120 DDR clock cycles = 7.8µs
 * Matches DDR3 tREFI specification exactly */
#define EMIF_REF_CTRL 0x00000C30

/* EMIF SDRAM Configuration - Memory controller settings
 * Configuration for DDR3 with 16-bit bus on AM335x:
 *   SDRAM_TYPE: DDR3
 *   IBANK_POS: bank address before column
 *   DDR_TERM: RZQ/4 (ODT termination)
 *   CWL: CAS Write Latency = 5
 *   NARROW_MODE: 16-bit bus (required for AM335x)
 *   CL: CAS Latency = 6
 *   ROWSIZE: 15-bit row address (32K rows)
 *   IBANK: 8 internal banks
 *   PAGESIZE: 10-bit column (1024 columns) */
#define EMIF_CFG    0x61C05332

/* DDR PHY Control - Physical layer settings
 *   PHY_ENABLE_DYNAMIC_PWRDN: 1 (power saving between bursts)
 *   READ_LATENCY: 7 (CAS latency 6 + 1 pipeline stage) */
#define DDR_PHY_CTRL    0x00100007

/* IO Impedance Control for DDR pads
 * Reference value for DDR signal integrity */
#define DDR_IO_CTRL_VAL 0x0000018B

/* VTP (Voltage Temperature Impedance Compensation) control bits */
#define VTP_CTRL_ENABLE   (1 << 6)  /* CLRZ: 0=reset, 1=enable compensation */
#define VTP_CTRL_READY    (1 << 5)  /* READY: set when calibration complete */

/* ============================================================
 * DDR3 Initialization Sequence
 * ============================================================ */

void ddr_init(void)
{
    uint32_t i;
    uart_putc('6'); /* CHECKPOINT 6: Start DDR Init */

    /* --------------------------------------------------------
     * Step 1: Enable VTP (Voltage Temperature Impedance Compensation)
     * VTP calibrates DDR pad impedance for signal integrity.
     * Sequence:
     * 1. Reset VTP (CLRZ=0)
     * 2. Enable VTP (CLRZ=1) to start calibration
     * 3. Wait for calibration complete (READY=1)
     * -------------------------------------------------------- */
    /* Step 1a: Force VTP reset */
    writel(readl(VTP_CTRL) & ~VTP_CTRL_ENABLE, VTP_CTRL);

    /* Step 1b: Enable VTP to start calibration */
    writel(readl(VTP_CTRL) | VTP_CTRL_ENABLE, VTP_CTRL);

    /* Step 1c: Wait for VTP calibration complete */
    for (i = 0; i < 10000; i++) {
        if (readl(VTP_CTRL) & VTP_CTRL_READY) break;
    }
    uart_putc('v'); /* CHECKPOINT v: VTP Calibration Done */

    uart_putc('7'); /* CHECKPOINT 7: VTP Loop Done */

    /* --------------------------------------------------------
     * Step 2: Configure DDR IO Control
     * Set impedance control for all DDR pad groups
     * -------------------------------------------------------- */
    writel(DDR_IO_CTRL_VAL, DDR_IO_CTRL);        /* DDR command pads */
    writel(DDR_IO_CTRL_VAL, DDR_DATA0_IOCTRL);   /* DDR data byte 0 pads */
    writel(DDR_IO_CTRL_VAL, DDR_DATA1_IOCTRL);   /* DDR data byte 1 pads */

    /* Enable CKE (Clock Enable) control */
    writel(0x1, DDR_CKE_CTRL);

    /* --------------------------------------------------------
     * Step 3: DDR PHY Initialization (Slave Ratios)
     * Critical timing ratios for 400MHz operation
     * -------------------------------------------------------- */
    /* Command macro timing ratios */
    writel(0x80, DDR_CMD0_SLAVE_RATIO_0);
    writel(0x80, DDR_CMD1_SLAVE_RATIO_0);
    writel(0x80, DDR_CMD2_SLAVE_RATIO_0);
    
    writel(0x00, DDR_CMD0_INVERT_CLKOUT_0);
    writel(0x00, DDR_CMD1_INVERT_CLKOUT_0);
    writel(0x00, DDR_CMD2_INVERT_CLKOUT_0);

    /* DATA0 timing ratios (byte 0) */
    writel(0x38, DDR_DATA0_RD_DQS_SLAVE_RATIO_0);
    writel(0x44, DDR_DATA0_WR_DQS_SLAVE_RATIO_0);
    writel(0x94, DDR_DATA0_FIFO_WE_SLAVE_RATIO_0);
    writel(0x7D, DDR_DATA0_WR_DATA_SLAVE_RATIO_0);
    writel(0x00, DDR_DATA0_GATE_LEVEL_INIT_RATIO_0); 

    /* DATA1 timing ratios (byte 1) */
    writel(0x38, DDR_DATA1_RD_DQS_SLAVE_RATIO_0);
    writel(0x44, DDR_DATA1_WR_DQS_SLAVE_RATIO_0);
    writel(0x94, DDR_DATA1_FIFO_WE_SLAVE_RATIO_0);
    writel(0x7D, DDR_DATA1_WR_DATA_SLAVE_RATIO_0);
    writel(0x00, DDR_DATA1_GATE_LEVEL_INIT_RATIO_0);

    /* --------------------------------------------------------
     * Step 4: Configure EMIF (External Memory Interface)
     * -------------------------------------------------------- */
    
    /* Set DDR PHY control registers */
    writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_1);
    writel(DDR_PHY_CTRL, EMIF_DDR_PHY_CTRL_2);

    /* Set timing parameters */
    writel(EMIF_TIM1, EMIF_SDRAM_TIM_1);
    writel(EMIF_TIM2, EMIF_SDRAM_TIM_2);
    writel(EMIF_TIM3, EMIF_SDRAM_TIM_3);

    /* Set refresh rate: 7.8µs (DDR3 specification) */
    writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);

    /* Configure SDRAM controller */
    writel(EMIF_CFG, EMIF_SDRAM_CONFIG);

    /* --------------------------------------------------------
     * Step 5: Wait for DDR initialization to complete
     * EMIF automatically runs DDR3 initialization sequence
     * Delay allows ZQ calibration and mode register programming
     * -------------------------------------------------------- */
    for (i = 0; i < 5000; i++);  /* ~500us delay */

    uart_putc('9'); /* CHECKPOINT 9: DDR Config OK */

    /* Re-enable refresh after initialization */
    writel(EMIF_REF_CTRL, EMIF_SDRAM_REF_CTRL);

    /* Note: If memory test fails, software leveling may be required */
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