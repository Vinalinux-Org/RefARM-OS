/* ============================================================
 * vfs.c
 * ------------------------------------------------------------
 * Virtual File System Implementation
 * ============================================================ */

#include "vfs.h"
#include "ramfs.h"
#include "syscalls.h"
#include "uart.h"
#include <stddef.h>

/* Forward declaration */
static struct vfs_operations *vfs_find_fs(const char *path);

/* ============================================================
 * File Descriptor Table
 * ============================================================ */

struct vfs_fd {
    bool     in_use;        /* FD allocated? */
    uint32_t file_index;    /* Index into ramfs_file_table */
    uint32_t offset;        /* Current read position */
};

static struct vfs_fd fd_table[MAX_FDS];

/* ============================================================
 * Helper: String Comparison
 * ============================================================ */

static int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* ============================================================
 * VFS Mount Table
 * ============================================================ */

struct vfs_mount {
    const char *mount_point;
    struct vfs_operations *fs_ops;
    bool in_use;
};

#define MAX_MOUNTS 4
static struct vfs_mount mount_table[MAX_MOUNTS];

/* ============================================================
 * VFS Initialization
 * ============================================================ */

void vfs_init(void)
{
    uart_printf("[VFS] Initializing Virtual File System...\n");
    
    /* Initialize FD table */
    for (int i = 0; i < MAX_FDS; i++) {
        fd_table[i].in_use = false;
        fd_table[i].file_index = 0;
        fd_table[i].offset = 0;
    }
    
    /* Initialize mount table */
    for (int i = 0; i < MAX_MOUNTS; i++) {
        mount_table[i].mount_point = NULL;
        mount_table[i].fs_ops = NULL;
        mount_table[i].in_use = false;
    }
    
    uart_printf("[VFS] Initialization complete\n");
}

/**
 * Mount filesystem at mount point
 */
int vfs_mount(const char *mount_point, struct vfs_operations *fs_ops)
{
    uart_printf("[VFS] Mounting filesystem at '%s'...\n", mount_point);
    
    /* Find free mount slot */
    int slot = -1;
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mount_table[i].in_use) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        uart_printf("[VFS] ERROR: No free mount slots\n");
        return E_FAIL;
    }
    
    /* Register filesystem */
    mount_table[slot].mount_point = mount_point;
    mount_table[slot].fs_ops = fs_ops;
    mount_table[slot].in_use = true;
    
    uart_printf("[VFS] Filesystem mounted at '%s'\n", mount_point);
    return E_OK;
}

/**
 * Find filesystem for path
 */
static struct vfs_operations *vfs_find_fs(const char *path)
{
    /* Simple implementation: only support root "/" */
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mount_table[i].in_use && 
            strcmp(mount_table[i].mount_point, "/") == 0) {
            return mount_table[i].fs_ops;
        }
    }
    return NULL;
}

/* ============================================================
 * File Operations
 * ============================================================ */

/**
 * Open a file
 */
int vfs_open(const char *path, int flags)
{
    /* Only support O_RDONLY */
    if (flags != O_RDONLY) {
        return E_ARG;
    }
    
    /* Find filesystem for path */
    struct vfs_operations *fs_ops = vfs_find_fs(path);
    if (!fs_ops) {
        return E_NOENT;
    }
    
    /* Strip leading '/' if present */
    const char *filename = path;
    if (*filename == '/') {
        filename++;
    }
    
    /* Lookup file in filesystem */
    int file_index = fs_ops->lookup(filename);
    if (file_index < 0) {
        return E_NOENT;  /* File not found */
    }
    
    /* Allocate file descriptor */
    int fd = -1;
    for (int i = 3; i < MAX_FDS; i++) {  /* Reserve 0-2 for stdin/stdout/stderr */
        if (!fd_table[i].in_use) {
            fd = i;
            break;
        }
    }
    
    if (fd < 0) {
        return E_MFILE;  /* Too many open files */
    }
    
    /* Initialize FD */
    fd_table[fd].in_use = true;
    fd_table[fd].file_index = file_index;
    fd_table[fd].offset = 0;
    
    return fd;
}

/**
 * Read from file descriptor
 */
int vfs_read(int fd, void *buf, uint32_t len)
{
    /* Validate FD */
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].in_use) {
        return E_BADF;
    }
    
    /* Find filesystem (assume root for now) */
    struct vfs_operations *fs_ops = vfs_find_fs("/");
    if (!fs_ops) {
        return E_FAIL;
    }
    
    /* Read from filesystem */
    int bytes_read = fs_ops->read(
        fd_table[fd].file_index,
        fd_table[fd].offset,
        buf,
        len
    );
    
    if (bytes_read > 0) {
        fd_table[fd].offset += bytes_read;
    }
    
    return bytes_read;
}

/**
 * Close file descriptor
 */
int vfs_close(int fd)
{
    /* Validate FD */
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].in_use) {
        return E_BADF;
    }
    
    /* Free FD */
    fd_table[fd].in_use = false;
    fd_table[fd].file_index = 0;
    fd_table[fd].offset = 0;
    
    return E_OK;
}

/**
 * List directory contents
 */
int vfs_listdir(const char *path, void *entries, uint32_t max_entries)
{
    file_info_t *file_entries = (file_info_t *)entries;
    
    /* Find filesystem for path */
    struct vfs_operations *fs_ops = vfs_find_fs(path);
    if (!fs_ops) {
        return E_NOENT;
    }
    
    /* Only support root directory */
    if (strcmp(path, "/") != 0) {
        return E_NOENT;
    }
    
    /* Get file count */
    int file_count = fs_ops->get_file_count();
    if (file_count < 0) {
        return file_count;
    }
    
    /* Fill entries */
    int count = 0;
    for (int i = 0; i < file_count && count < (int)max_entries; i++) {
        int ret = fs_ops->get_file_info(
            i,
            file_entries[count].name,
            &file_entries[count].size
        );
        
        if (ret == E_OK) {
            count++;
        }
    }
    
    return count;
}
