#ifndef _USER_LIB_H
#define _USER_LIB_H

#include <stdint.h>

/* ============================================================
 * User Space System Call Wrappers
 * ============================================================ */

/* Yield CPU to another task */
int sys_yield(void);

/* Write data to UART */
int sys_write(const void *buf, uint32_t len);

/* Terminate current task */
void sys_exit(int status);

#endif /* _USER_LIB_H */
