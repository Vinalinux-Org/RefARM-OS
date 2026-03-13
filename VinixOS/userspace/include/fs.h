/* ============================================================
 * fs.h
 * ------------------------------------------------------------
 * Filesystem syscall wrappers for user applications
 * ============================================================ */

#ifndef FS_H
#define FS_H

#include "types.h"
#include "syscalls.h"

/* ============================================================
 * File Operations
 * ============================================================ */

/**
 * Open a file
 * 
 * @param path File path (e.g., "/README.txt")
 * @param flags Open flags (O_RDONLY only supported)
 * @return File descriptor (>= 0) on success, negative error code on failure
 */
int sys_open(const char *path, int flags);

/**
 * Read from file descriptor
 * 
 * @param fd File descriptor
 * @param buf Buffer to read into
 * @param len Number of bytes to read
 * @return Number of bytes read (>= 0), or negative error code
 */
int sys_read_file(int fd, void *buf, uint32_t len);

/**
 * Close file descriptor
 * 
 * @param fd File descriptor
 * @return 0 on success, negative error code on failure
 */
int sys_close(int fd);

/**
 * List directory contents
 * 
 * @param path Directory path (e.g., "/")
 * @param entries Buffer to store file_info_t entries
 * @param max_entries Maximum number of entries to return
 * @return Number of entries filled (>= 0), or negative error code
 */
int sys_listdir(const char *path, file_info_t *entries, uint32_t max_entries);

#endif /* FS_H */