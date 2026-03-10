/* ============================================================
 * ramfs.h
 * ------------------------------------------------------------
 * RAM Filesystem (Read-Only, Embedded Files)
 * ============================================================ */

#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>

/* ============================================================
 * RAMFS File Structure
 * ============================================================ */

struct ramfs_file {
    const char    *name;    /* File name (null-terminated) */
    const uint8_t *data;    /* Pointer to embedded data */
    uint32_t       size;    /* File size in bytes */
};

/* ============================================================
 * RAMFS Operations
 * ============================================================ */

/**
 * Initialize RAMFS (compute file sizes)
 * Called during VFS initialization
 * 
 * @return 0 on success, negative error code on failure
 */
int ramfs_init(void);

/**
 * Get RAMFS operations structure
 * 
 * @return Pointer to ramfs operations
 */
struct vfs_operations *ramfs_get_operations(void);

/**
 * Lookup file by name
 * 
 * @param name File name (e.g., "README.txt")
 * @return File index (>= 0) on success, negative error code on failure
 */
int ramfs_lookup(const char *name);

/**
 * Read from file
 * 
 * @param file_index Index into ramfs_file_table
 * @param offset Offset within file
 * @param buf Buffer to read into
 * @param len Number of bytes to read
 * @return Number of bytes read (>= 0), or negative error code
 */
int ramfs_read(int file_index, uint32_t offset, void *buf, uint32_t len);

/**
 * Get file count
 * 
 * @return Number of files in RAMFS
 */
int ramfs_get_file_count(void);

/**
 * Get file info by index
 * 
 * @param index File index
 * @param name_out Buffer to store file name (min 32 bytes)
 * @param size_out Pointer to store file size
 * @return 0 on success, negative error code on failure
 */
int ramfs_get_file_info(int index, char *name_out, uint32_t *size_out);

#endif /* RAMFS_H */
