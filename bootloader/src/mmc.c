/* ============================================================
 * mmc.c
 * Simple MMC/SD card driver
 * 
 * Supports: RAW sector read only
 * No filesystem support (bootloader reads raw sectors)
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

static int sdhc_card = 0;

/* MMC Status bits (AM335x TRM MMCHS_STAT Register) */
#define MMC_STAT_CC         BIT(0)   /* Command complete */
#define MMC_STAT_TC         BIT(1)   /* Transfer complete */
#define MMC_STAT_ERRI       BIT(15)  /* Error interrupt */
#define MMC_STAT_BRR        BIT(5)   /* Buffer read ready */

/* MMC Command flags (AM335x TRM MMCHS_CMD Register) */
#define MMC_CMD_RSP_NONE    (0 << 16)
#define MMC_CMD_RSP_136     (1 << 16)
#define MMC_CMD_RSP_48      (2 << 16)
#define MMC_CMD_RSP_48_BUSY (3 << 16)

#define MMC_CMD_CCCE        BIT(19) /* Command CRC Check Enable */
#define MMC_CMD_CICE        BIT(20) /* Command Index Check Enable */
#define MMC_CMD_DP          BIT(21) /* Data Present */
#define MMC_CMD_DDIR_READ   BIT(4)  /* Data Direction: Read */
#define MMC_CMD_DDIR_WRITE  (0)     /* Data Direction: Write */

/* Response combinations */
#define MMC_RSP_NONE        (MMC_CMD_RSP_NONE)
#define MMC_RSP_R1          (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R1b         (MMC_CMD_RSP_48_BUSY | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R2          (MMC_CMD_RSP_136 | MMC_CMD_CCCE)
#define MMC_RSP_R3          (MMC_CMD_RSP_48)
#define MMC_RSP_R6          (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_RSP_R7          (MMC_CMD_RSP_48 | MMC_CMD_CCCE | MMC_CMD_CICE)

/* ============================================================
 * Helper: Send MMC command
 * ============================================================ */
static int mmc_send_cmd(uint32_t cmd, uint32_t arg, uint32_t flags)
{
    uint32_t status;

    /* Clear status */
    writel(0xFFFFFFFF, MMC_STAT);

    /* Set argument */
    writel(arg, MMC_ARG);

    /* Send command */
    writel((cmd << 24) | flags, MMC_CMD);

    int timeout = 10000000;
    /* Wait for command complete */
    do {
        status = readl(MMC_STAT);
        if (--timeout == 0) {
            return -1;
        }
    } while ((status & (MMC_STAT_CC | MMC_STAT_ERRI)) == 0);

    /* Check for error */
    if (status & MMC_STAT_ERRI) {
        /* Per SD spec: must reset CMD line after any command error.
         * Without SRC reset, the controller CMD state machine remains
         * stuck in error state and ignores subsequent commands. */
        uint32_t sysctl = readl(MMC_SYSCTL);
        writel(sysctl | MMC_SYSCTL_SRC, MMC_SYSCTL);
        /* SRC is self-clearing — wait for it */
        int srctimeout = 100000;
        while ((readl(MMC_SYSCTL) & MMC_SYSCTL_SRC) && (--srctimeout > 0));
        /* Clear all error status bits */
        writel(0xFFFFFFFF, MMC_STAT);
        return -1;
    }

    /* Clear status */
    writel(MMC_STAT_CC, MMC_STAT);

    return 0;
}

/* ============================================================
 * MMC Initialization
 * ============================================================ */
int mmc_init(void)
{
    uint32_t rsp;
    uart_putc('a'); /* CHECKPOINT a: mmc_init entry */

    /* --------------------------------------------------------
     * Configure MMC0 pins
     * -------------------------------------------------------- */
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT3);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT2);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT1);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_DAT0);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_CLK);
    writel(PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN, CONF_MMC0_CMD);

    /* --------------------------------------------------------
     * Reset MMC controller
     * -------------------------------------------------------- */
    writel(0x02, MMC_SYSCONFIG);
    int timeout = 1000000;
    while ((readl(MMC_SYSSTATUS) & 0x01) == 0) {
        if (--timeout == 0) { 
            uart_puts("MMC:    Timeout waiting for SYSSTATUS reset\r\n");
            return -1; 
        }
    }

    /* Set SYSCONFIG to keep clocks active (TRM Ch18 Step 5.6)
     * After SOFTRESET, SYSCONFIG=0 → CLOCKACTIVITY=0 → functional clock
     * (CLKADPI 96MHz) gets gated off by power manager → 80-clock init fails.
     * CLOCKACTIVITY[9:8]=11 = both OCP + functional clocks active during idle
     * SIDLEMODE[4:3]=01 = no-idle (power manager cannot gate module)
     * AUTOIDLE[0]=0 = free-running (no auto clock gating) */
    writel(0x00000308, MMC_SYSCONFIG);

    /* --------------------------------------------------------
     * Initialize MMC controller
     * -------------------------------------------------------- */
    
    /* Set bus width to 1-bit */
    writel(0x00000000, MMC_CON);

    /* Soft Reset CMD (SRC) and DAT (SRD) lines  (TRM 18.4.1.28) */
    writel(readl(MMC_SYSCTL) | (1 << 25) | (1 << 26), MMC_SYSCTL);
    timeout = 10000000;
    while (readl(MMC_SYSCTL) & ((1 << 25) | (1 << 26))) {
        if (--timeout == 0) return -1;
    }

    /* Set capabilities — TRM Ch18 Step 3 (BEFORE voltage/power setup)
     * SD_CAPA[24] = VS33 = 1: tell controller it supports 3.3V
     * Without this, HCTL.SDVS=7 (3.3V) may be rejected and SDBP stays 0 */
    writel(readl(MMC_CAPA) | (1 << 24), MMC_CAPA);

    /* Set bus voltage to 3.3V (TRM MMCHS_HCTL SDVS[11:9]=111=3.3V)
     * Step 1: Set voltage WITHOUT enabling power (SDBP=0) */
    writel(MMC_HCTL_SDVS_3_3V, MMC_HCTL);  /* SDVS=3.3V, SDBP=0 */

    /* Step 2: Enable bus power (SDBP=bit8=1) — must be done AFTER SDVS */
    writel(readl(MMC_HCTL) | MMC_HCTL_SDBP, MMC_HCTL);

    /* Verify SDBP is actually set (bus power on) */
    timeout = 100000;
    while ((readl(MMC_HCTL) & MMC_HCTL_SDBP) == 0) {
        if (--timeout == 0) { 
            uart_puts("MMC:    Failed to enable bus power (SDBP)\r\n");
            return -1; 
        }
    }

    /* Enable internal clock, CLKD=240 → 96MHz/240 = 400KHz, DTO=0xE (longest timeout)
     * Bits [15:6]=CLKD: 240<<6 = 0x3C00
     * Bits [19:16]=DTO: 0xE = TCF*(2^27) = maximum timeout
     * Bit [0]=ICE=1, Bit [2]=CEN=0 (enable CEN only after ICS stable) */
    writel(0x000E3C01, MMC_SYSCTL);  /* ICE=1, CEN=0, CLKD=240, DTO=14 */
    delay(1000);

    /* Wait for Internal Clock Stable (ICS, bit 1) */
    timeout = 1000000;
    while ((readl(MMC_SYSCTL) & MMC_SYSCTL_ICS) == 0) {
        if (--timeout == 0) { 
            uart_puts("MMC:    Timeout waiting for Internal Clock Stable\r\n");
            return -1; 
        }
    }

    /* Enable clock output to card (CEN, bit 2) ONLY after ICS=1 */
    writel(readl(MMC_SYSCTL) | MMC_SYSCTL_CEN, MMC_SYSCTL);

    /* Enable interrupts (CRITICAL for polled mode: must enable status bits!)
     * MMC_IE enables the hardware to set bits in MMC_STAT.
     * MMC_ISE is optional for polling, but good practice. */
    writel(0xFFFFFFFF, MMC_IE);
    writel(0xFFFFFFFF, MMC_ISE);
    writel(0xFFFFFFFF, MMC_STAT); /* Clear any pending status */

    /* --------------------------------------------------------
     * Mandatory 80-clock initialization sequence (TRM Ch18)
     * Set SD_CON[INIT]=1, send dummy CMD0 (arg=0, no response)
     * The controller sends ≥80 clock pulses to bring card
     * out of power-on undefined state before any real command.
     * Without this, cmd0/cmd8/cmd41 are unreliable.
     * -------------------------------------------------------- */
    writel(readl(MMC_CON) | (1 << 1), MMC_CON);  /* SD_CON[INIT]=1 */
    writel(0x00000000, MMC_CMD);                  /* Dummy CMD0 (raw write, not mmc_send_cmd) */
    delay(10000);                                 /* Wait for 80+ clocks at 400KHz (~200µs) */

    /* Wait for CC (init sequence complete) */
    timeout = 1000000;
    while ((readl(MMC_STAT) & BIT(0)) == 0) {    /* CC bit = bit 0 */
        if (--timeout == 0) { 
            uart_puts("MMC:    Timeout waiting for 80-clock init\r\n");
            return -1; 
        }
    }
    writel(BIT(0), MMC_STAT);                     /* Clear CC */
    writel(readl(MMC_CON) & ~(1 << 1), MMC_CON); /* SD_CON[INIT]=0 */
    delay(5000);                                  /* Wait ≥1ms after init */
    writel(0xFFFFFFFF, MMC_STAT);                 /* Clear all status */

    /* CMD0: GO_IDLE_STATE — actual reset to idle
     * Now the card is in well-defined power-on state, CMD0 will be ACKed fast */
    mmc_send_cmd(0, 0, MMC_RSP_NONE);
    delay(5000);                                  /* Wait ≥1ms per SD spec */

    /* CMD8: SEND_IF_COND — check if card supports SDv2 voltage range
     * Arg: VHS=1 (2.7-3.6V), check pattern=0xAA → 0x01AA
     * CTO error here = SDv1 card or no card (not fatal, continue with SDv1 flow) */
    mmc_send_cmd(8, 0x1AA, MMC_RSP_R7);

    /* ACMD41: Send operating conditions */
    /* Note: ACMD = CMD55 + CMD41 */
    for (int i = 0; i < 2000; i++) {
        /* CMD55: APP_CMD - R1 */
        if (mmc_send_cmd(55, 0x00000000, MMC_RSP_R1) != 0) {
            continue;
        }

        /* ACMD41: SD_SEND_OP_COND - R3
         * Arg: HCS=1 (bit 30) for SDHC support, Voltage=3.3V (bit 20) */
        if (mmc_send_cmd(41, 0x40300000, MMC_RSP_R3) != 0) {
            continue;
        }

        /* Check if power up is done (bit 31 of response) */
        if (readl(MMC_RSP10) & (1U << 31)) {
            /* Card is ready! Read CCS (bit 30) to see if SDHC */
            if (readl(MMC_RSP10) & (1 << 30)) {
                sdhc_card = 1; /* Block addressing */
            }
            break;
        }
        delay(1000); // Wait before retry
    }

    /* CMD2: ALL_SEND_CID - R2 */
    mmc_send_cmd(2, 0, MMC_RSP_R2);

    /* CMD3: SET_REL_ADDR - R6 */
    mmc_send_cmd(3, 0, MMC_RSP_R6);
    rsp = readl(MMC_RSP10);
    uint32_t rca = (rsp >> 16) & 0xFFFF;

    /* CMD9: SEND_CSD - R2 */
    mmc_send_cmd(9, rca << 16, MMC_RSP_R2);

    /* CMD7: SELECT_CARD - R1b */
    mmc_send_cmd(7, rca << 16, MMC_RSP_R1b);

    /* Set Data Timeout (DTO) to max: 14 = TCF*2^27 (TRM 18.4.1.28)
     * Keep clock at 400KHz for now or jump to 25MHz */
    uint32_t sysctl = readl(MMC_SYSCTL);
    sysctl = (sysctl & ~(0xF << 16)) | (14 << 16);
    writel(sysctl, MMC_SYSCTL);

    /* Increase clock to ~25 MHz
     * CLKD (Clock Divider) = FCLK / TargetCLK = 96MHz / 25MHz ≈ 4
     * (MMCHS FCLK = 96MHz from PER PLL M2/2)
     *
     * Safe sequence (TRM 18.4.1.28):
     *   1. Clear CEN (disable clock to card)
     *   2. Write new CLKD divider
     *   3. Wait for ICS (Internal Clock Stable)
     *   4. Set CEN (re-enable clock to card)
     *
     * CRITICAL: Must NOT use |= for CLKD bits — old 400KHz divider
     * value from init (CLKD=0xA0=160 → ~600KHz) would stay set.
     */
    {
        uint32_t sysctl;

        /* Step 1: Disable clock to card (keep ICE=1, CEN=0) */
        sysctl = readl(MMC_SYSCTL);
        sysctl &= ~(1 << 2);    /* Clear CEN (bit 2) */
        writel(sysctl, MMC_SYSCTL);

        /* Step 2: Set new divider. CLKD = bits [25:6]. Set CLKD=4 for ~25MHz */
        sysctl = readl(MMC_SYSCTL);
        sysctl &= ~(0xFFC0);    /* Clear CLKD bits [15:6] (10 bits) */
        sysctl |= (4 << 6);     /* CLKD = 4 */
        writel(sysctl, MMC_SYSCTL);

        /* Step 3: Wait for Internal Clock Stable (ICS, bit 1) */
        timeout = 1000000;
        while ((readl(MMC_SYSCTL) & 0x0002) == 0) {
            if (--timeout == 0) return -1;
        }

        /* Step 4: Enable clock to card */
        writel(readl(MMC_SYSCTL) | (1 << 2), MMC_SYSCTL);
    }

    /* CMD16: SET_BLOCKLEN = 512 bytes
     * Required for SDSC cards (CCS=0).
     * SDHC/SDXC cards ignore this but it's harmless.
     * TRM: Must be done after SELECT_CARD (CMD7) */
    mmc_send_cmd(16, 512, MMC_RSP_R1);

    return 0;
}

/* ============================================================
 * Read sectors from SD card
 * ============================================================ */
int mmc_read_sectors(uint32_t start_sector, uint32_t count, void *dest)
{
    uint32_t *buf = (uint32_t *)dest;
    uint32_t i, j;

    for (i = 0; i < count; i++) {
        /* Set block length (BLEN) and block count (NBLK) in MMCHS_BLK
         * NBLK[31:16] = 1 block, BLEN[15:0] = 512 bytes
         * TRM: Chapter 18 - MMCHS_BLK register */
        writel((1 << 16) | 512, MMC_BLK);

        /* Clear status ready for this transfer */
        writel(0xFFFFFFFF, MMC_STAT);

        /* Send CMD17: READ_SINGLE_BLOCK - R1 with Data
         * DDIR=1 (read), DP=1 (data present) 
         * Argument depends on card type:
         * SDSC: byte address (start_sector * 512)
         * SDHC/SDXC: block number (start_sector) */
        uint32_t arg = sdhc_card ? (start_sector + i) : ((start_sector + i) * 512);
        writel(arg, MMC_ARG);
        writel((17 << 24) | MMC_RSP_R1 | MMC_CMD_DP | MMC_CMD_DDIR_READ, MMC_CMD);

        /* Wait for buffer read ready */
        int timeout = 10000000;
        while ((readl(MMC_STAT) & MMC_STAT_BRR) == 0) {
            uint32_t stat = readl(MMC_STAT);
            if (stat & MMC_STAT_ERRI) {
                panic("MMC Read ERR");
                return -1;
            }
            if (--timeout == 0) {
                panic("MMC Read BRR T/O");
                return -1;
            }
        }

        /* Read 512 bytes (128 words) */
        for (j = 0; j < 128; j++) {
            *buf++ = readl(MMC_DATA);
        }

        /* Note: After reading exactly BLEN bytes (128 words), 
         * the controller clears BRR automatically. Wait for TC. */

        /* Wait for transfer complete */
        timeout = 10000000;
        while ((readl(MMC_STAT) & MMC_STAT_TC) == 0) {
            if (--timeout == 0) {
                panic("MMC Read TC T/O");
                return -1;
            }
        }

        /* Clear status */
        writel(0xFFFFFFFF, MMC_STAT);
    }

    return 0;
}