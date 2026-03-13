/* ============================================================
 * idle.h
 * ------------------------------------------------------------
 * Idle task header for VinixOS
 * ============================================================ */

#ifndef IDLE_H
#define IDLE_H

#include "task.h"

/**
 * Get idle task structure
 * @return Pointer to idle task
 */
struct task_struct *get_idle_task(void);

#endif /* IDLE_H */
