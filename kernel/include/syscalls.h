#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <stdint.h>

/* ============================================================
 * System Call Numbers (ABI Contract)
 * ============================================================ */
#define SYS_WRITE   0
#define SYS_EXIT    1
#define SYS_YIELD   2
#define SYS_READ    3
#define SYS_GET_TASKS   4
#define SYS_GET_MEMINFO 5
#define SYS_OPEN        6
#define SYS_READ_FILE   7
#define SYS_CLOSE       8
#define SYS_LISTDIR     9

/* ============================================================
 * Data Structures
 * ============================================================ */

typedef struct {
    uint32_t id;
    char name[32];
    uint32_t state;
} process_info_t;

typedef struct {
    uint32_t total;       // Total RAM (e.g. 128MB)
    uint32_t free;        // Approximate free heap
    uint32_t kernel_text;
    uint32_t kernel_data;
    uint32_t kernel_bss;
    uint32_t kernel_stack;
} mem_info_t;

typedef struct {
    char name[32];        // File name
    uint32_t size;        // File size in bytes
} file_info_t;

/* ============================================================
 * Error Codes
 * ============================================================ */
#define E_OK        0       /* Success */
#define E_FAIL      -1      /* Generic failure */
#define E_INVAL     -2      /* Invalid syscall number */
#define E_ARG       -3      /* Invalid argument */
#define E_PTR       -4      /* Invalid pointer (User Memory Rule) */
#define E_PERM      -5      /* Permission denied */
#define E_NOENT     -6      /* No such file or directory */
#define E_BADF      -7      /* Bad file descriptor */
#define E_MFILE     -8      /* Too many open files */

/* ============================================================
 * File Open Flags
 * ============================================================ */
#define O_RDONLY    0       /* Read only */
#define O_WRONLY    1       /* Write only (not supported) */
#define O_RDWR      2       /* Read/Write (not supported) */

#endif /* _SYSCALLS_H */
