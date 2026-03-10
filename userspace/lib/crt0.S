/* ============================================================
 * crt0.S
 * ------------------------------------------------------------
 * User Application Startup Code for RefARM-OS
 * ============================================================
 * Execution Flow:
 * 1. Kernel creates task and jumps to _start (0x40000000)
 *    - Initial Stack Pointer is set up by Kernel
 * 2. Clear .bss section
 * 3. Call C entry point (main)
 * 4. If main returns, call sys_exit
 * ============================================================ */

.syntax unified
.text

/* Entry point */
.global _start
.section .text.crt0

_start:
    /* Kernel already sets SP for this task
     * We don't need to initialize the stack pointer here */

    /* 1. Clear BSS */
    ldr r0, =_bss_start
    ldr r1, =_bss_end
    mov r2, #0
1:
    cmp r0, r1
    bhs 2f
    str r2, [r0], #4
    b 1b
2:

    /* 2. Call main() */
    bl main

    /* 3. Call sys_exit(status)
     * R0 contains return value from main(), which we pass to sys_exit
     * sys_exit is an assembly wrapper in syscall.c */
    bl sys_exit

    /* Should never reach here */
3:
    b 3b
