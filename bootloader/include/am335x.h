/* ============================================================
 * am335x.h
 * AM335x SoC Register Definitions
 * ============================================================ */

#ifndef AM335X_H
#define AM335X_H

#include "types.h"

/* ============================================================
 * Memory Map
 * ============================================================ */

/* On-chip RAM (SRAM) */
#define SRAM_BASE           0x402F0400
#define SRAM_SIZE           0x0001BC00    /* 109 KB */

/* DDR3 SDRAM */
#define DDR_BASE            0x80000000
#define DDR_SIZE            0x10000000    /* 256 MB */

/* ============================================================
 * Control Module
 * Base: 0x44E10000
 * ============================================================ */
#define CONTROL_MODULE_BASE 0x44E10000

/* Pin Mux Registers */
#define CONF_UART0_RXD      (CONTROL_MODULE_BASE + 0x970)
#define CONF_UART0_TXD      (CONTROL_MODULE_BASE + 0x974)
#define CONF_MMC0_DAT3      (CONTROL_MODULE_BASE + 0x8F0)
#define CONF_MMC0_DAT2      (CONTROL_MODULE_BASE + 0x8F4)
#define CONF_MMC0_DAT1      (CONTROL_MODULE_BASE + 0x8F8)
#define CONF_MMC0_DAT0      (CONTROL_MODULE_BASE + 0x8FC)
#define CONF_MMC0_CLK       (CONTROL_MODULE_BASE + 0x900)
#define CONF_MMC0_CMD       (CONTROL_MODULE_BASE + 0x904)

/* Pin Mux Mode Values */
#define PIN_MODE_0          0
#define PIN_MODE_1          1
#define PIN_MODE_2          2
#define PIN_MODE_3          3

/* Pin Configuration Bits */
#define PIN_INPUT_EN        (1 << 5)     /* Input enable */
#define PIN_PULLUP_EN       (1 << 4)     /* Pull-up enable */
#define PIN_PULLDOWN_DIS    (0 << 3)     /* Pull-down disable */
#define PIN_RXACTIVE        (1 << 5)     /* Receiver active */

/* ============================================================
 * Clock Module (CM)
 * Base: 0x44E00000
 * ============================================================ */
#define CM_BASE             0x44E00000

/* CM_PER - Peripheral Clock Management */
#define CM_PER_BASE         (CM_BASE + 0x000)
#define CM_PER_L4LS_CLKSTCTRL   (CM_PER_BASE + 0x00)  /* L4LS clock domain control */
#define CM_PER_L3_CLKSTCTRL     (CM_PER_BASE + 0x04)  /* L3 clock domain control */
#define CM_PER_L4FW_CLKSTCTRL   (CM_PER_BASE + 0x08)  /* L4FW clock domain control */
#define CM_PER_EMIF_CLKCTRL     (CM_PER_BASE + 0x28)
#define CM_PER_MMC0_CLKCTRL     (CM_PER_BASE + 0x3C)

/* WKUP Clocks */
#define CM_WKUP_BASE        0x44E00400
#define CM_WKUP_UART0_CLKCTRL (CM_WKUP_BASE + 0xB4)

/* CM_DPLL - DPLL Configuration */
#define CM_DPLL_BASE        (CM_BASE + 0x500)

/* MPU PLL (500 MHz) */
#define CM_CLKMODE_DPLL_MPU     (CM_DPLL_BASE + 0x88)
#define CM_IDLEST_DPLL_MPU      (CM_DPLL_BASE + 0x20)
#define CM_CLKSEL_DPLL_MPU      (CM_DPLL_BASE + 0x2C)
#define CM_DIV_M2_DPLL_MPU      (CM_DPLL_BASE + 0xA8)

/* DDR PLL (400 MHz) */
#define CM_CLKMODE_DPLL_DDR     (CM_DPLL_BASE + 0x94)
#define CM_IDLEST_DPLL_DDR      (CM_DPLL_BASE + 0x34)
#define CM_CLKSEL_DPLL_DDR      (CM_DPLL_BASE + 0x40)
#define CM_DIV_M2_DPLL_DDR      (CM_DPLL_BASE + 0xA0)

/* CORE PLL (1000 MHz) */
#define CM_CLKMODE_DPLL_CORE    (CM_DPLL_BASE + 0x90)
#define CM_IDLEST_DPLL_CORE     (CM_DPLL_BASE + 0x5C)
#define CM_CLKSEL_DPLL_CORE     (CM_DPLL_BASE + 0x68)
#define CM_DIV_M4_DPLL_CORE     (CM_DPLL_BASE + 0x80)
#define CM_DIV_M5_DPLL_CORE     (CM_DPLL_BASE + 0x84)
#define CM_DIV_M6_DPLL_CORE     (CM_DPLL_BASE + 0xD8)

/* PER PLL (960 MHz) */
#define CM_CLKMODE_DPLL_PER     (CM_DPLL_BASE + 0x8C)
#define CM_IDLEST_DPLL_PER      (CM_DPLL_BASE + 0x70)
#define CM_CLKSEL_DPLL_PER      (CM_DPLL_BASE + 0x9C)
#define CM_DIV_M2_DPLL_PER      (CM_DPLL_BASE + 0xAC)

/* DPLL Mode Values */
#define DPLL_MN_BYP_MODE    4    /* MN bypass mode */
#define DPLL_LP_BYP_MODE    5    /* Low-power bypass mode */
#define DPLL_FR_BYP_MODE    6    /* Fast relock bypass mode */
#define DPLL_LOCK_MODE      7    /* Lock mode */

/* Module Enable Values */
#define MODULE_DISABLE      0
#define MODULE_ENABLE       2

/* ============================================================
 * UART0
 * Base: 0x44E09000
 * ============================================================ */
#define UART0_BASE          0x44E09000

#define UART0_THR           (UART0_BASE + 0x00)  /* Transmit Holding Register */
#define UART0_RHR           (UART0_BASE + 0x00)  /* Receive Holding Register */
#define UART0_DLL           (UART0_BASE + 0x00)  /* Divisor Latch Low */
#define UART0_DLH           (UART0_BASE + 0x04)  /* Divisor Latch High */
#define UART0_IER           (UART0_BASE + 0x04)  /* Interrupt Enable Register */
#define UART0_FCR           (UART0_BASE + 0x08)  /* FIFO Control Register */
#define UART0_LCR           (UART0_BASE + 0x0C)  /* Line Control Register */
#define UART0_MCR           (UART0_BASE + 0x10)  /* Modem Control Register */
#define UART0_LSR           (UART0_BASE + 0x14)  /* Line Status Register */
#define UART0_MDR1          (UART0_BASE + 0x20)  /* Mode Definition Register 1 */
#define UART0_SYSC          (UART0_BASE + 0x54)  /* System Configuration */
#define UART0_SYSS          (UART0_BASE + 0x58)  /* System Status */

/* UART Line Control Register (LCR) bits */
#define UART_LCR_DIV_EN     (1 << 7)             /* Divisor latch enable */
#define UART_LCR_8N1        0x03                 /* 8 data bits, no parity, 1 stop */

/* UART Line Status Register (LSR) bits */
#define UART_LSR_RXFIFOE    (1 << 0)             /* RX FIFO not empty */
#define UART_LSR_TXFIFOE    (1 << 5)             /* TX FIFO empty */
#define UART_LSR_TEMT       (1 << 6)             /* Transmitter empty (FIFO + shift reg) */

/* UART Mode Definition Register (MDR1) values */
#define UART_16X_MODE       0x00
#define UART_DISABLE        0x07

/* ============================================================
 * EMIF (External Memory Interface) for DDR3
 * Base: 0x4C000000
 * ============================================================ */
#define EMIF_BASE           0x4C000000

#define EMIF_STATUS         (EMIF_BASE + 0x04)
#define EMIF_SDRAM_CONFIG   (EMIF_BASE + 0x08)
#define EMIF_SDRAM_CONFIG2  (EMIF_BASE + 0x0C)
#define EMIF_SDRAM_REF_CTRL (EMIF_BASE + 0x10)
#define EMIF_SDRAM_REF_CTRL_SHDW (EMIF_BASE + 0x14)
#define EMIF_SDRAM_TIM_1    (EMIF_BASE + 0x18)  /* TRM Ch7: offset 0x018 */
#define EMIF_SDRAM_TIM_2    (EMIF_BASE + 0x20)  /* TRM Ch7: offset 0x020 */
#define EMIF_SDRAM_TIM_3    (EMIF_BASE + 0x28)  /* TRM Ch7: offset 0x028 */
#define EMIF_PWR_MGMT_CTRL  (EMIF_BASE + 0x38)
#define EMIF_DDR_PHY_CTRL_1 (EMIF_BASE + 0xE4)
#define EMIF_DDR_PHY_CTRL_2 (EMIF_BASE + 0xEC)  /* TRM: shadow reg at 0xEC */

/* DDR PHY Control */
/* DDR PHY Control */
#define DDR_PHY_BASE        0x44E10000  /* Control Module Base */
#define DDR_CMD0_IOCTRL     (DDR_PHY_BASE + 0x1404)
#define DDR_CMD1_IOCTRL     (DDR_PHY_BASE + 0x1408)
#define DDR_CMD2_IOCTRL     (DDR_PHY_BASE + 0x140C)
#define DDR_DATA0_IOCTRL    (DDR_PHY_BASE + 0x1440)
#define DDR_DATA1_IOCTRL    (DDR_PHY_BASE + 0x1444)

/* DDR PHY Slave Ratios (Required for 400MHz) */
#define DDR_CMD0_SLAVE_RATIO_0              (CONTROL_MODULE_BASE + 0x0C1C)
#define DDR_CMD0_INVERT_CLKOUT_0            (CONTROL_MODULE_BASE + 0x0C2C)
#define DDR_CMD1_SLAVE_RATIO_0              (CONTROL_MODULE_BASE + 0x0C50)
#define DDR_CMD1_INVERT_CLKOUT_0            (CONTROL_MODULE_BASE + 0x0C60)
#define DDR_CMD2_SLAVE_RATIO_0              (CONTROL_MODULE_BASE + 0x0C84)
#define DDR_CMD2_INVERT_CLKOUT_0            (CONTROL_MODULE_BASE + 0x0C94)

#define DDR_DATA0_RD_DQS_SLAVE_RATIO_0      (CONTROL_MODULE_BASE + 0x0CC8)
#define DDR_DATA0_WR_DQS_SLAVE_RATIO_0      (CONTROL_MODULE_BASE + 0x0CD4)
#define DDR_DATA0_FIFO_WE_SLAVE_RATIO_0     (CONTROL_MODULE_BASE + 0x0CD8)
#define DDR_DATA0_WR_DATA_SLAVE_RATIO_0     (CONTROL_MODULE_BASE + 0x0CDC)
#define DDR_DATA0_GATE_LEVEL_INIT_RATIO_0   (CONTROL_MODULE_BASE + 0x0CE4)

#define DDR_DATA1_RD_DQS_SLAVE_RATIO_0      (CONTROL_MODULE_BASE + 0x0D0C)
#define DDR_DATA1_WR_DQS_SLAVE_RATIO_0      (CONTROL_MODULE_BASE + 0x0D18)
#define DDR_DATA1_FIFO_WE_SLAVE_RATIO_0     (CONTROL_MODULE_BASE + 0x0D1C)
#define DDR_DATA1_WR_DATA_SLAVE_RATIO_0     (CONTROL_MODULE_BASE + 0x0D20)
#define DDR_DATA1_GATE_LEVEL_INIT_RATIO_0   (CONTROL_MODULE_BASE + 0x0D28)

#define DDR_IO_CTRL         (CONTROL_MODULE_BASE + 0xE04)
#define VTP_CTRL            (CONTROL_MODULE_BASE + 0xE0C)
#define DDR_CKE_CTRL        (CONTROL_MODULE_BASE + 0x131C)

/* ============================================================
 * Watchdog Timer 1 (WDT1)
 * Base: 0x44E35000
 * ============================================================ */
#define WDT1_BASE           0x44E35000
#define WDT1_WSPR           (WDT1_BASE + 0x48)
#define WDT1_WWPS           (WDT1_BASE + 0x34)

/* ============================================================
 * MMC/SD Controller
 * Base: 0x48060000 (MMC0)
 * ============================================================ */
#define MMC0_BASE           0x48060000

#define MMC_SYSCONFIG       (MMC0_BASE + 0x110)
#define MMC_SYSSTATUS       (MMC0_BASE + 0x114)
#define MMC_CON             (MMC0_BASE + 0x12C)
#define MMC_BLK             (MMC0_BASE + 0x204)
#define MMC_ARG             (MMC0_BASE + 0x208)
#define MMC_CMD             (MMC0_BASE + 0x20C)
#define MMC_RSP10           (MMC0_BASE + 0x210)
#define MMC_DATA            (MMC0_BASE + 0x220)
#define MMC_PSTATE          (MMC0_BASE + 0x224)   /* Present State */
#define MMC_HCTL            (MMC0_BASE + 0x228)   /* Host Control */
#define MMC_SYSCTL          (MMC0_BASE + 0x22C)   /* System Control */
#define MMC_STAT            (MMC0_BASE + 0x230)   /* Interrupt Status (Flags, write 1 to clear) */
#define MMC_IE              (MMC0_BASE + 0x234)   /* Interrupt Enable (enables STAT bits) */
#define MMC_ISE             (MMC0_BASE + 0x238)   /* Interrupt Signal Enable */
#define MMC_AC12            (MMC0_BASE + 0x23C)
#define MMC_CAPA            (MMC0_BASE + 0x240)
#define MMC_CUR_CAPA        (MMC0_BASE + 0x248)   /* Current Capabilities */
#define MMC_ADMAES          (MMC0_BASE + 0x254)   /* ADMA Error Status */

/* MMC_HCTL Bitmasks */
#define MMC_HCTL_DTW_4BIT   (1 << 1)
#define MMC_HCTL_HSPE       (1 << 2)
#define MMC_HCTL_SDBP       (1 << 8)    /* SD Bus Power */
#define MMC_HCTL_SDVS_1_8V  (0x5 << 9)  /* SD Bus Voltage 1.8V */
#define MMC_HCTL_SDVS_3_0V  (0x6 << 9)  /* SD Bus Voltage 3.0V */
#define MMC_HCTL_SDVS_3_3V  (0x7 << 9)  /* SD Bus Voltage 3.3V */

/* MMC_SYSCTL Bitmasks */
#define MMC_SYSCTL_ICE      (1 << 0)    /* Internal Clock Enable */
#define MMC_SYSCTL_ICS      (1 << 1)    /* Internal Clock Stable */
#define MMC_SYSCTL_CEN      (1 << 2)    /* Clock Enable */
#define MMC_SYSCTL_CLKD_SHIFT 6         /* Clock Divider Shift */
#define MMC_SYSCTL_SRA      (1 << 24)   /* Software Reset For All */
#define MMC_SYSCTL_SRC      (1 << 25)   /* Software Reset For CMD line */
#define MMC_SYSCTL_SRD      (1 << 26)   /* Software Reset For DAT line */

/* MMC Command Types */
#define MMC_CMD_GO_IDLE         0   /* CMD0 */
#define MMC_CMD_SEND_OP_COND    1   /* CMD1 */
#define MMC_CMD_ALL_SEND_CID    2   /* CMD2 */
#define MMC_CMD_SET_REL_ADDR    3   /* CMD3 */
#define MMC_CMD_SELECT_CARD     7   /* CMD7 */
#define MMC_CMD_SEND_IF_COND    8   /* CMD8 */
#define MMC_CMD_SEND_CSD        9   /* CMD9 */
#define MMC_CMD_READ_SINGLE     17  /* CMD17 */
#define MMC_CMD_READ_MULTIPLE   18  /* CMD18 */

/* ============================================================
 * Register Access Macros
 * ============================================================ */
#define REG32(addr)         (*(volatile uint32_t *)(addr))
#define REG16(addr)         (*(volatile uint16_t *)(addr))
#define REG8(addr)          (*(volatile uint8_t *)(addr))

#define readl(addr)         REG32(addr)
#define writel(val, addr)   (REG32(addr) = (val))

#define readw(addr)         REG16(addr)
#define writew(val, addr)   (REG16(addr) = (val))

#define readb(addr)         REG8(addr)
#define writeb(val, addr)   (REG8(addr) = (val))

/* Bit manipulation */
#define BIT(n)              (1U << (n))
#define SETBIT(reg, bit)    ((reg) |= BIT(bit))
#define CLRBIT(reg, bit)    ((reg) &= ~BIT(bit))
#define GETBIT(reg, bit)    (((reg) >> (bit)) & 1)

#endif /* AM335X_H */