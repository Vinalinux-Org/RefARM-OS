/* ============================================================
 * vfs.h
 * ------------------------------------------------------------
 * Virtual File System Interface
 * ============================================================ */

#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Constants
 * ============================================================ */
#define MAX_FDS         16      /* Maximum open file descriptors */
#define MAX_PATH        256     /* Maximum path length */

/* ============================================================
 * VFS Operations Structure
 * ============================================================ */

struct vfs_operations {
    int (*lookup)(const char *name);
    int (*read)(int file_index, uint32_t offset, void *buf, uint32_t len);
    int (*get_file_count)(void);
    int (*get_file_info)(int index, char *name_out, uint32_t *size_out);
};

/* ============================================================
 * VFS Initialization
 * ============================================================ */

/**
 * Initialize VFS layer
 * Must be called during kernel initialization
 */
void vfs_init(void);

/**
 * Mount filesystem at mount point
 * 
 * @param mount_point Mount point (e.g., "/")
 * @param fs_ops Filesystem operations structure
 * @return 0 on success, negative error code on failure
 */
int vfs_mount(const char *mount_point, struct vfs_operations *fs_ops);

/* ============================================================
 * File Operations
 * ============================================================ */

/**
 * Open a file
 * 
 * @param path File path (e.g., "/README.txt")
 * @param flags Open flags (O_RDONLY, etc.)
 * @return File descriptor (>= 0) on success, negative error code on failure
 */
int vfs_open(const char *path, int flags);

/**
 * Read from file descriptor
 * 
 * @param fd File descriptor
 * @param buf Buffer to read into
 * @param len Number of bytes to read
 * @return Number of bytes read (>= 0), or negative error code
 */
int vfs_read(int fd, void *buf, uint32_t len);

/**
 * Close file descriptor
 * 
 * @param fd File descriptor
 * @return 0 on success, negative error code on failure
 */
int vfs_close(int fd);

/**
 * List directory contents
 * 
 * @param path Directory path (e.g., "/")
 * @param entries Buffer to store file_info_t entries
 * @param max_entries Maximum number of entries to return
 * @return Number of entries filled (>= 0), or negative error code
 */
int vfs_listdir(const char *path, void *entries, uint32_t max_entries);

#endif /* VFS_H */
