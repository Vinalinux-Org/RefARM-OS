#ifndef _USER_SYSCALL_H
#define _USER_SYSCALL_H

#include <stdint.h>

/* ============================================================
 * User Mode System Call Wrappers
 * ============================================================ */

/* 
 * Yield CPU to another task 
 * Returns: 0 on success
 */
int sys_yield(void);

/* 
 * Write data to standard output (UART)
 * buf: Pointer to data
 * len: Length in bytes
 * Returns: Number of bytes written or error code
 */
int sys_write(const void *buf, uint32_t len);

/* 
 * Terminate current task
 * status: Exit status code
 * Returns: Does not return
 */
void sys_exit(int status);

/* 
 * Read data from standard input (UART)
 * buf: Pointer to buffer
 * len: Max length to read
 * Returns: Number of bytes read or error code
 */
int sys_read(void *buf, uint32_t len);

#endif /* _USER_SYSCALL_H */
