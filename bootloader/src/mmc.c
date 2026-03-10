/* ============================================================
 * mmc.c
 * Simple MMC/SD card driver for AM335x
 * 
 * Supports: Raw sector read only (no filesystem)
 * Used by bootloader to load kernel from SD card
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

static int sdhc_card = 0;

/* MMC Status register bits */
#define MMC_STAT_CC         BIT(0)   /* Command complete */
#define MMC_STAT_TC         BIT(1)   /* Transfer complete */
#define MMC_STAT_ERRI       BIT(15)  /* Error interrupt */
#define MMC_STAT_BRR        BIT(5)   /* Buffer read ready */

/* MMC Command register flags */
#define MMC_CMD_RSP_NONE    (0 << 16)  /* No response */
#define MMC_CMD_RSP_136     (1 << 16)  /* 136-bit response */
#define MMC_CMD_RSP_48      (2 << 16)  /* 48-bit response */
#define MMC_CMD_RSP_48_BUSY (3 << 16)  /* 48-bit response with busy */

#define MMC_CMD_CCCE        BIT(19) /* Command CRC Check Enable */
#define MMC_CMD_CICE        BIT(20) /* Command Index Check Enable */
#define MMC_CMD_DP          BIT(21) /* Data Present */
#define MMC_CMD_DDIR_READ   BIT(4)  /* Data Direction: Read */
#define MMC_CMD_DDIR_WRITE  (0)     /* Data Direction: Write */

/* Response type combinations */
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

    /* Clear status register */
    writel(0xFFFFFFFF, MMC_STAT);

    /* Set command argument */
    writel(arg, MMC_ARG);

    /* Send command with flags */
    writel((cmd << 24) | flags, MMC_CMD);

    int timeout = 10000000;
    /* Wait for command complete or error */
    do {
        status = readl(MMC_STAT);
        if (--timeout == 0) {
            return -1;  /* Timeout */
        }
    } while ((status & (MMC_STAT_CC | MMC_STAT_ERRI)) == 0);

    /* Check for error */
    if (status & MMC_STAT_ERRI) {
        /* Reset command line after error (required by SD specification)
         * Without reset, controller may remain stuck in error state */
        uint32_t sysctl = readl(MMC_SYSCTL);
        writel(sysctl | MMC_SYSCTL_SRC, MMC_SYSCTL);
        
        /* Wait for reset to complete (self-clearing bit) */
        int srctimeout = 100000;
        while ((readl(MMC_SYSCTL) & MMC_SYSCTL_SRC) && (--srctimeout > 0));
        
        /* Clear all error status bits */
        writel(0xFFFFFFFF, MMC_STAT);
        return -1;  /* Command error */
    }

    /* Clear command complete status */
    writel(MMC_STAT_CC, MMC_STAT);

    return 0;  /* Success */
}

/* ============================================================
 * MMC Initialization
 * ============================================================ */
int mmc_init(void)
{
    uint32_t rsp;
    uart_putc('a'); /* CHECKPOINT a: mmc_init entry */

    /* --------------------------------------------------------
     * Configure MMC0 pins (SD card interface)
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

    /* Configure SYSCONFIG to keep clocks active
     * Prevents power manager from gating clocks during initialization */
    writel(0x00000308, MMC_SYSCONFIG);

    /* --------------------------------------------------------
     * Initialize MMC controller
     * -------------------------------------------------------- */
    
    /* Set bus width to 1-bit (default) */
    writel(0x00000000, MMC_CON);

    /* Soft reset command and data lines */
    writel(readl(MMC_SYSCTL) | (1 << 25) | (1 << 26), MMC_SYSCTL);
    timeout = 10000000;
    while (readl(MMC_SYSCTL) & ((1 << 25) | (1 << 26))) {
        if (--timeout == 0) return -1;
    }

    /* Set controller capabilities: support 3.3V operation */
    writel(readl(MMC_CAPA) | (1 << 24), MMC_CAPA);

    /* Set bus voltage to 3.3V (step 1: voltage without power) */
    writel(MMC_HCTL_SDVS_3_3V, MMC_HCTL);

    /* Enable bus power (step 2: power after voltage) */
    writel(readl(MMC_HCTL) | MMC_HCTL_SDBP, MMC_HCTL);

    /* Verify bus power is enabled */
    timeout = 100000;
    while ((readl(MMC_HCTL) & MMC_HCTL_SDBP) == 0) {
        if (--timeout == 0) { 
            uart_puts("MMC:    Failed to enable bus power (SDBP)\r\n");
            return -1; 
        }
    }

    /* Enable internal clock at 400KHz (initialization frequency)
     * CLKD=240: 96MHz/240 = 400KHz
     * DTO=14: maximum timeout value */
    writel(0x000E3C01, MMC_SYSCTL);  /* ICE=1, CEN=0, CLKD=240, DTO=14 */
    delay(1000);

    /* Wait for internal clock to stabilize */
    timeout = 1000000;
    while ((readl(MMC_SYSCTL) & MMC_SYSCTL_ICS) == 0) {
        if (--timeout == 0) { 
            uart_puts("MMC:    Timeout waiting for Internal Clock Stable\r\n");
            return -1; 
        }
    }

    /* Enable clock output to SD card */
    writel(readl(MMC_SYSCTL) | MMC_SYSCTL_CEN, MMC_SYSCTL);

    /* Enable status interrupts (required for polled mode) */
    writel(0xFFFFFFFF, MMC_IE);
    writel(0xFFFFFFFF, MMC_ISE);
    writel(0xFFFFFFFF, MMC_STAT); /* Clear any pending status */

    /* --------------------------------------------------------
     * 80-clock initialization sequence (SD card requirement)
     * Sends at least 80 clock pulses to wake up SD card
     * -------------------------------------------------------- */
    writel(readl(MMC_CON) | (1 << 1), MMC_CON);  /* SD_CON[INIT]=1 */
    writel(0x00000000, MMC_CMD);                  /* Dummy CMD0 */
    delay(10000);                                 /* Wait for 80+ clocks */

    /* Wait for initialization complete */
    timeout = 1000000;
    while ((readl(MMC_STAT) & BIT(0)) == 0) {
        if (--timeout == 0) { 
            uart_puts("MMC:    Timeout waiting for 80-clock init\r\n");
            return -1; 
        }
    }
    writel(BIT(0), MMC_STAT);                     /* Clear status */
    writel(readl(MMC_CON) & ~(1 << 1), MMC_CON); /* SD_CON[INIT]=0 */
    delay(5000);                                  /* Wait after init */
    writel(0xFFFFFFFF, MMC_STAT);                 /* Clear all status */

    /* CMD0: Reset card to idle state */
    mmc_send_cmd(0, 0, MMC_RSP_NONE);
    delay(5000);

    /* CMD8: Check if card supports SDv2 voltage range
     * If error, card may be SDv1 (not fatal) */
    mmc_send_cmd(8, 0x1AA, MMC_RSP_R7);

    /* ACMD41: Send operating conditions (initialize card) */
    for (int i = 0; i < 2000; i++) {
        /* CMD55: APP_CMD (prefix for application commands) */
        if (mmc_send_cmd(55, 0x00000000, MMC_RSP_R1) != 0) {
            continue;
        }

        /* ACMD41: SD_SEND_OP_COND (initialize with SDHC support) */
        if (mmc_send_cmd(41, 0x40300000, MMC_RSP_R3) != 0) {
            continue;
        }

        /* Check if card is ready (bit 31) */
        if (readl(MMC_RSP10) & (1U << 31)) {
            /* Check if card supports SDHC (bit 30) */
            if (readl(MMC_RSP10) & (1 << 30)) {
                sdhc_card = 1; /* SDHC card (block addressing) */
            }
            break;
        }
        delay(1000); /* Wait before retry */
    }

    /* Standard SD card initialization sequence */
    mmc_send_cmd(2, 0, MMC_RSP_R2);  /* CMD2: Get CID */
    mmc_send_cmd(3, 0, MMC_RSP_R6);  /* CMD3: Get RCA */
    rsp = readl(MMC_RSP10);
    uint32_t rca = (rsp >> 16) & 0xFFFF;

    mmc_send_cmd(9, rca << 16, MMC_RSP_R2);  /* CMD9: Get CSD */
    mmc_send_cmd(7, rca << 16, MMC_RSP_R1b); /* CMD7: Select card */

    /* Set maximum data timeout */
    uint32_t sysctl = readl(MMC_SYSCTL);
    sysctl = (sysctl & ~(0xF << 16)) | (14 << 16);
    writel(sysctl, MMC_SYSCTL);

    /* Increase clock speed to ~25 MHz (operational speed) */
    {
        uint32_t sysctl;

        /* Step 1: Disable clock to card */
        sysctl = readl(MMC_SYSCTL);
        sysctl &= ~(1 << 2);    /* Clear CEN */
        writel(sysctl, MMC_SYSCTL);

        /* Step 2: Set new divider for 25MHz (96MHz/4 = 24MHz) */
        sysctl = readl(MMC_SYSCTL);
        sysctl &= ~(0xFFC0);    /* Clear CLKD bits */
        sysctl |= (4 << 6);     /* CLKD = 4 */
        writel(sysctl, MMC_SYSCTL);

        /* Step 3: Wait for internal clock to stabilize */
        timeout = 1000000;
        while ((readl(MMC_SYSCTL) & 0x0002) == 0) {
            if (--timeout == 0) return -1;
        }

        /* Step 4: Enable clock to card */
        writel(readl(MMC_SYSCTL) | (1 << 2), MMC_SYSCTL);
    }

    /* CMD16: Set block length to 512 bytes (required for SDSC cards) */
    mmc_send_cmd(16, 512, MMC_RSP_R1);

    return 0;  /* Initialization successful */
}

/* ============================================================
 * Read sectors from SD card
 * ============================================================ */
int mmc_read_sectors(uint32_t start_sector, uint32_t count, void *dest)
{
    uint32_t *buf = (uint32_t *)dest;
    uint32_t i, j;

    for (i = 0; i < count; i++) {
        /* Configure block transfer: 1 block of 512 bytes */
        writel((1 << 16) | 512, MMC_BLK);

        /* Clear status register */
        writel(0xFFFFFFFF, MMC_STAT);

        /* Send READ_SINGLE_BLOCK command (CMD17)
         * Argument format depends on card type:
         * - SDSC: byte address = sector * 512
         * - SDHC/SDXC: block number = sector */
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

        /* Read 512 bytes (128 words) from data register */
        for (j = 0; j < 128; j++) {
            *buf++ = readl(MMC_DATA);
        }

        /* Wait for transfer complete */
        timeout = 10000000;
        while ((readl(MMC_STAT) & MMC_STAT_TC) == 0) {
            if (--timeout == 0) {
                panic("MMC Read TC T/O");
                return -1;
            }
        }

        /* Clear status for next transfer */
        writel(0xFFFFFFFF, MMC_STAT);
    }

    return 0;  /* Success */
}