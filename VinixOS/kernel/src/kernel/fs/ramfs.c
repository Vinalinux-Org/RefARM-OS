/* ============================================================
 * ramfs.c
 * ------------------------------------------------------------
 * RAM Filesystem Implementation (Read-Only, Embedded Files)
 * ============================================================ */

#include "ramfs.h"
#include "vfs.h"
#include "syscalls.h"
#include "uart.h"
#include "string.h"
#include "types.h"
#include "../../build/ramfs_generated.h"

/* ============================================================
 * Helper: Minimum
 * ============================================================ */

static uint32_t min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

/* ============================================================
 * RAMFS Operations
 * ============================================================ */

/**
 * Mount RAMFS (initialize file table)
 */
int ramfs_init(void)
{
    uart_printf("[RAMFS] Initializing embedded file table...\n");
    
    /* Compute file sizes from start/end pointers */
    for (uint32_t i = 0; i < ramfs_file_count; i++) {
        ramfs_file_table[i].size = (uint32_t)(ramfs_file_end_table[i] - ramfs_file_table[i].data);
        uart_printf("[RAMFS]   %s: %u bytes at 0x%08x\n",
                    ramfs_file_table[i].name,
                    ramfs_file_table[i].size,
                    (uint32_t)ramfs_file_table[i].data);
    }
    
    uart_printf("[RAMFS] File table ready: %u files\n", ramfs_file_count);
    return E_OK;
}

/* ============================================================
 * RAMFS Operations Structure
 * ============================================================ */

static struct vfs_operations ramfs_ops = {
    .lookup = ramfs_lookup,
    .read = ramfs_read,
    .get_file_count = ramfs_get_file_count,
    .get_file_info = ramfs_get_file_info
};

/**
 * Get RAMFS operations structure
 */
struct vfs_operations *ramfs_get_operations(void)
{
    return &ramfs_ops;
}

/**
 * Lookup file by name
 */
int ramfs_lookup(const char *name)
{
    for (uint32_t i = 0; i < ramfs_file_count; i++) {
        if (strcmp(ramfs_file_table[i].name, name) == 0) {
            return (int)i;  /* Return file index */
        }
    }
    
    return E_NOENT;  /* File not found */
}

/**
 * Read from file
 */
int ramfs_read(int file_index, uint32_t offset, void *buf, uint32_t len)
{
    /* Validate file index */
    if (file_index < 0 || (uint32_t)file_index >= ramfs_file_count) {
        return E_BADF;
    }
    
    struct ramfs_file *file = &ramfs_file_table[file_index];
    
    /* Check offset bounds */
    if (offset >= file->size) {
        return 0;  /* EOF */
    }
    
    /* Calculate bytes to read */
    uint32_t bytes_to_read = min(len, file->size - offset);
    
    /* Copy data */
    memcpy(buf, file->data + offset, bytes_to_read);
    
    return (int)bytes_to_read;
}

/**
 * Get file count
 */
int ramfs_get_file_count(void)
{
    return (int)ramfs_file_count;
}

/**
 * Get file info by index
 */
int ramfs_get_file_info(int index, char *name_out, uint32_t *size_out)
{
    /* Validate index */
    if (index < 0 || (uint32_t)index >= ramfs_file_count) {
        return E_BADF;
    }
    
    struct ramfs_file *file = &ramfs_file_table[index];
    
    /* Copy name (ensure null termination) */
    strcpy(name_out, file->name);
    
    /* Copy size */
    *size_out = file->size;
    
    return E_OK;
}