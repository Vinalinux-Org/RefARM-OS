/* ============================================================
 * trace.h
 * ------------------------------------------------------------
 * Trace macros for internal kernel observability.
 * TRACE != LOG: Trace is for reasoning about execution flow.
 * ============================================================ */

#ifndef TRACE_H
#define TRACE_H

#include "uart.h"

/* Configuration Switch */
#define SCHED_TRACE_ENABLE  1

/* External dependency for timestamp (if available) */
extern volatile uint32_t timer_get_ticks(void);

#if SCHED_TRACE_ENABLE
    #define TRACE_SCHED(fmt, ...) \
        uart_printf("[SCHED] " fmt "\n", ##__VA_ARGS__)
        /* Note: Can add timestamp here later: "[%u] [SCHED]..." */
#else
    #define TRACE_SCHED(fmt, ...) do { } while (0)
#endif

#endif /* TRACE_H */
