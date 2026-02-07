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


/* 
 * Get list of tasks
 * buf: Pointer to array of process_info_t
 * max_count: Max tasks to retrieve
 * Returns: Number of tasks filled, or error
 */
int sys_get_tasks(void *buf, uint32_t max_count);

/* 
 * Get memory information
 * buf: Pointer to mem_info_t
 * Returns: 0 on success, or error
 */
int sys_get_meminfo(void *buf);

#endif /* _USER_SYSCALL_H */
