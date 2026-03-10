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

/* ============================================================
 * External Symbols (Defined in payload.S)
 * ============================================================ */

/* hello.txt */
extern uint8_t _ramfs_hello_start;
extern uint8_t _ramfs_hello_end;

/* info.txt */
extern uint8_t _ramfs_info_start;
extern uint8_t _ramfs_info_end;

/* ============================================================
 * RAMFS File Table
 * ============================================================ */

static struct ramfs_file ramfs_file_table[] = {
    {
        .name = "hello.txt", 
        .data = &_ramfs_hello_start,
        .size = 0  /* Computed at mount time */
    },
    {
        .name = "info.txt",
        .data = &_ramfs_info_start,
        .size = 0  /* Computed at mount time */
    },
    {
        .name = NULL,  /* Sentinel */
        .data = NULL,
        .size = 0
    }
};

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
    
    /* Compute file sizes */
    ramfs_file_table[0].size = (uint32_t)&_ramfs_hello_end - (uint32_t)&_ramfs_hello_start;
    ramfs_file_table[1].size = (uint32_t)&_ramfs_info_end - (uint32_t)&_ramfs_info_start;
    
    /* Count files and display info */
    int file_count = 0;
    for (int i = 0; ramfs_file_table[i].name != NULL; i++) {
        uart_printf("[RAMFS]   %s: %u bytes at 0x%08x\n",
                    ramfs_file_table[i].name,
                    ramfs_file_table[i].size,
                    (uint32_t)ramfs_file_table[i].data);
        file_count++;
    }
    
    uart_printf("[RAMFS] File table ready: %d files\n", file_count);
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
    for (int i = 0; ramfs_file_table[i].name != NULL; i++) {
        if (strcmp(ramfs_file_table[i].name, name) == 0) {
            return i;  /* Return file index */
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
    if (file_index < 0 || ramfs_file_table[file_index].name == NULL) {
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
    int count = 0;
    for (int i = 0; ramfs_file_table[i].name != NULL; i++) {
        count++;
    }
    return count;
}

/**
 * Get file info by index
 */
int ramfs_get_file_info(int index, char *name_out, uint32_t *size_out)
{
    /* Validate index */
    if (index < 0 || ramfs_file_table[index].name == NULL) {
        return E_BADF;
    }
    
    struct ramfs_file *file = &ramfs_file_table[index];
    
    /* Copy name (ensure null termination) */
    strcpy(name_out, file->name);
    
    /* Copy size */
    *size_out = file->size;
    
    return E_OK;
}