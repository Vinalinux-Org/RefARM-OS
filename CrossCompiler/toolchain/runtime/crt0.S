/*
 * C Runtime Startup (crt0.S)
 * Entry point for user programs on VinixOS Platform
 */

.syntax unified
.arch armv7-a

.section .text
.global _start

_start:
    /* Call main function */
    bl main
    
    /* Exit with return value from main */
    /* r0 already contains return value */
    mov r7, #1      /* SYS_EXIT = 1 */
    svc #0          /* Invoke syscall */
    
    /* Should never reach here */
1:  b 1b
