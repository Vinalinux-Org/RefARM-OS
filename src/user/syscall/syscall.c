#include "user_syscall.h"
#include "syscalls.h"

/* ============================================================
 * MINIMAL VERSION: Pure Assembly with No Optimization
 * ============================================================
 * This version uses the simplest possible assembly to ensure
 * that GCC cannot optimize away the register setup.
 * 
 * We use separate functions instead of a helper to avoid any
 * potential inlining or optimization issues.
 */

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