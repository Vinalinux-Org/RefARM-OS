/*
 * Syscall Wrappers for VinixOS Platform
 * Provides C-callable wrappers for system calls
 */

.syntax unified
.arch armv7-a

.section .text

/* ============================================================
 * int write(int fd, const char* buf, int count)
 * ============================================================ */
.global write
.type write, %function
write:
    mov r7, #0      /* SYS_WRITE = 0 */
    svc #0
    bx lr

/* ============================================================
 * void exit(int status)
 * ============================================================ */
.global exit
.type exit, %function
exit:
    mov r7, #1      /* SYS_EXIT = 1 */
    svc #0
    /* Should never return */
1:  b 1b

/* ============================================================
 * void yield(void)
 * ============================================================ */
.global yield
.type yield, %function
yield:
    mov r7, #2      /* SYS_YIELD = 2 */
    svc #0
    bx lr

/* ============================================================
 * int read(int fd, char* buf, int count)
 * ============================================================ */
.global read
.type read, %function
read:
    mov r7, #3      /* SYS_READ = 3 */
    svc #0
    bx lr
