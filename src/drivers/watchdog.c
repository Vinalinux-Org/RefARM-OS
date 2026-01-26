/* ============================================================
 * watchdog.c
 * ------------------------------------------------------------
 * Watchdog timer driver implementation
 * Target: AM335x WDT1
 * ============================================================
 */

#include "watchdog.h"
#include "mmio.h"

/* ============================================================
 * Hardware definitions
 * ============================================================
 */
#define WDT1_BASE       0x44E35000
#define CM_WKUP_BASE    0x44E00400

/* Watchdog registers (offsets from WDT1_BASE) */
#define WDT_WSPR        0x48    /* Start/Stop Register */
#define WDT_WWPS        0x34    /* Write Posting Status */

/* Clock module registers */
#define CM_WKUP_WDT1_CLKCTRL    0xD4

/* Magic values for disable sequence */
#define WDT_DISABLE_SEQ1    0xAAAA
#define WDT_DISABLE_SEQ2    0x5555

/* ============================================================
 * Driver implementation
 * ============================================================
 */

void watchdog_disable(void)
{
    /*
     * Watchdog disable sequence per AM335x TRM 20.4.3.8:
     * 
     * CRITICAL: Must enable watchdog clock first before accessing registers!
     * Without this, writes to WDT registers will hang/fail.
     */
    
    /* Step 1: Enable watchdog module clock */
    mmio_write32(CM_WKUP_BASE + CM_WKUP_WDT1_CLKCTRL, 0x2);
    
    /* Wait for clock to be enabled (IDLEST bits = 0) */
    while ((mmio_read32(CM_WKUP_BASE + CM_WKUP_WDT1_CLKCTRL) & 0x3) != 0x2)
        ;
    
    /* Step 2: First disable sequence */
    mmio_write32(WDT1_BASE + WDT_WSPR, WDT_DISABLE_SEQ1);
    
    /* Wait for write to complete (WWPS register) */
    while (mmio_read32(WDT1_BASE + WDT_WWPS))
        ;
    
    /* Step 3: Second disable sequence */
    mmio_write32(WDT1_BASE + WDT_WSPR, WDT_DISABLE_SEQ2);
    
    /* Wait for write to complete */
    while (mmio_read32(WDT1_BASE + WDT_WWPS))
        ;
    
    /* Watchdog is now disabled */
}

/* Future implementations:
 * 
 * void watchdog_enable(void) {
 *     // Write enable sequence to WSPR
 *     // Configure timeout via WLDR (Load Register)
 * }
 * 
 * void watchdog_kick(void) {
 *     // Write trigger sequence to WTGR (Trigger Register)
 * }
 */