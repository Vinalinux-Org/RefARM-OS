#include "user_syscall.h"
#include "syscalls.h"

/* ============================================================
 * System Call Wrappers
 * ------------------------------------------------------------
 * Assembly wrappers for SVC instruction to ensure correct
 * register setup and avoid compiler optimization issues.
 * ============================================================ */

/* ============================================================
 * sys_yield - Yield CPU to another task
 * ============================================================ */
int sys_yield(void)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #2\n\t"         /* SYS_YIELD = 2 */
        "mov    r0, #0\n\t"          /* arg1 = 0 */
        "mov    r1, #0\n\t"          /* arg2 = 0 */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        :                            /* No inputs */
        : "r0", "r1", "r2", "r7"     /* Clobbers */
    );
    
    return ret;
}

/* ============================================================
 * sys_write - Write data to UART
 * ============================================================ */
int sys_write(const void *buf, uint32_t len)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #0\n\t"          /* SYS_WRITE = 0 */
        "mov    r0, %1\n\t"          /* arg1 = buf */
        "mov    r1, %2\n\t"          /* arg2 = len */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (buf), "r" (len)       /* Inputs: buf, len */
        : "r0", "r1", "r2", "r7"     /* Clobbers */
    );
    
    return ret;
}

/* ============================================================
 * sys_exit - Terminate current task
 * ============================================================ */
void sys_exit(int status)
{
    __asm__ __volatile__ (
        "mov    r7, #1\n\t"          /* SYS_EXIT = 1 */
        "mov    r0, %0\n\t"          /* arg1 = status */
        "mov    r1, #0\n\t"          /* arg2 = 0 */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        :                            /* No outputs */
        : "r" (status)               /* Input: status */
        : "r0", "r1", "r2", "r7"     /* Clobbers */
    );
    
    /* Should never return */
    while(1);
}

/* ============================================================
 * sys_read - Read data from UART
 * ============================================================ */
int sys_read(void *buf, uint32_t len)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #3\n\t"          /* SYS_READ = 3 */
        "mov    r0, %1\n\t"          /* arg1 = buf */
        "mov    r1, %2\n\t"          /* arg2 = len */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (buf), "r" (len)       /* Inputs: buf, len */
        /* Clobbers: r0-r3 are arguments/result, r7 is ID. 
         * "memory" is crucial because we write to buf */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}

/* ============================================================
 * sys_get_tasks - Get process list
 * ============================================================ */
int sys_get_tasks(void *buf, uint32_t max_count)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #4\n\t"          /* SYS_GET_TASKS = 4 */
        "mov    r0, %1\n\t"          /* arg1 = buf */
        "mov    r1, %2\n\t"          /* arg2 = max_count */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (buf), "r" (max_count) /* Inputs */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}

/* ============================================================
 * sys_get_meminfo - Get memory info
 * ============================================================ */
int sys_get_meminfo(void *buf)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #5\n\t"          /* SYS_GET_MEMINFO = 5 */
        "mov    r0, %1\n\t"          /* arg1 = buf */
        "mov    r1, #0\n\t"          /* arg2 = 0 */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (buf)                  /* Inputs */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}

/* ============================================================
 * Filesystem syscalls
 * ============================================================ */

/* ============================================================
 * sys_open - Open a file
 * ============================================================ */
int sys_open(const char *path, int flags)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #6\n\t"          /* SYS_OPEN = 6 */
        "mov    r0, %1\n\t"          /* arg1 = path */
        "mov    r1, %2\n\t"          /* arg2 = flags */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (path), "r" (flags)    /* Inputs */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}

/* ============================================================
 * sys_read_file - Read from file descriptor
 * ============================================================ */
int sys_read_file(int fd, void *buf, uint32_t len)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #7\n\t"          /* SYS_READ_FILE = 7 */
        "mov    r0, %1\n\t"          /* arg1 = fd */
        "mov    r1, %2\n\t"          /* arg2 = buf */
        "mov    r2, %3\n\t"          /* arg3 = len */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (fd), "r" (buf), "r" (len) /* Inputs */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}

/* ============================================================
 * sys_close - Close file descriptor
 * ============================================================ */
int sys_close(int fd)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #8\n\t"          /* SYS_CLOSE = 8 */
        "mov    r0, %1\n\t"          /* arg1 = fd */
        "mov    r1, #0\n\t"          /* arg2 = 0 */
        "mov    r2, #0\n\t"          /* arg3 = 0 */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (fd)                   /* Inputs */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}

/* ============================================================
 * sys_listdir - List directory contents
 * ============================================================ */
int sys_listdir(const char *path, void *entries, uint32_t max_entries)
{
    int ret;
    
    __asm__ __volatile__ (
        "mov    r7, #9\n\t"          /* SYS_LISTDIR = 9 */
        "mov    r0, %1\n\t"          /* arg1 = path */
        "mov    r1, %2\n\t"          /* arg2 = entries */
        "mov    r2, %3\n\t"          /* arg3 = max_entries */
        "svc    #0\n\t"              /* Trigger SVC */
        "mov    %0, r0\n\t"          /* Save return value */
        : "=r" (ret)                 /* Output: ret */
        : "r" (path), "r" (entries), "r" (max_entries) /* Inputs */
        : "r0", "r1", "r2", "r7", "memory"
    );
    
    return ret;
}