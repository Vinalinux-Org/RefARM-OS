/* ============================================================
 * test_task.h
 * ------------------------------------------------------------
 * Test task header for RefARM-OS
 * ============================================================ */

#ifndef TEST_TASK_H
#define TEST_TASK_H

#include "task.h"

/**
 * Get test task structure
 * @return Pointer to test task
 */
struct task_struct *get_test_task(void);

#endif /* TEST_TASK_H */
