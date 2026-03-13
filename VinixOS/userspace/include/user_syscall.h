#ifndef _USER_SYSCALL_H
#define _USER_SYSCALL_H

#include "types.h"

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

/* ============================================================
 * Filesystem syscalls
 * ============================================================ */

/*
 * Open a file
 * path: File path (e.g., "/README.txt")
 * flags: Open flags (O_RDONLY only supported)
 * Returns: File descriptor (>= 0) on success, negative error code on failure
 */
int sys_open(const char *path, int flags);

/*
 * Read from file descriptor
 * fd: File descriptor
 * buf: Buffer to read into
 * len: Number of bytes to read
 * Returns: Number of bytes read (>= 0), or negative error code
 */
int sys_read_file(int fd, void *buf, uint32_t len);

/*
 * Close file descriptor
 * fd: File descriptor
 * Returns: 0 on success, negative error code on failure
 */
int sys_close(int fd);

/*
 * List directory contents
 * path: Directory path (e.g., "/")
 * entries: Buffer to store file_info_t entries
 * max_entries: Maximum number of entries to return
 * Returns: Number of entries filled (>= 0), or negative error code
 */
int sys_listdir(const char *path, void *entries, uint32_t max_entries);

/*
 * Replace current process image with executable at path
 * path: Executable file path (ELF32 ARM)
 * Returns: Does not return on success, negative error code on failure
 */
int sys_exec(const char *path);

#endif /* _USER_SYSCALL_H */
