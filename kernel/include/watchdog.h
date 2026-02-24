/* ============================================================
 * watchdog.h
 * ------------------------------------------------------------
 * Watchdog timer driver interface
 * Target: AM335x WDT1
 * ============================================================
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

/* ============================================================
 * Public API
 * ============================================================
 */

/**
 * Disable watchdog timer
 * 
 * AM335x boots with watchdog enabled by ROM code.
 * This function disables it using the proper unlock sequence
 * per AM335x TRM Section 20.4.3.8
 * 
 * Must be called early in boot before watchdog expires (~3 seconds)
 */
void watchdog_disable(void);

/**
 * Enable watchdog timer (future)
 * TODO: Implement when needed for production systems
 */
/* void watchdog_enable(void); */

/**
 * Kick/refresh watchdog timer (future)
 * TODO: Implement when watchdog is used
 */
/* void watchdog_kick(void); */

#endif /* WATCHDOG_H */