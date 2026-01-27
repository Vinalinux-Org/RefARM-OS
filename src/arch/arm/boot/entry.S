/* ============================================================
 * entry.S
 * ------------------------------------------------------------
 * Entry point after reset handler.
 *
 * Responsibilities:
 *  - Clear .bss section
 *  - Transfer control to kernel_main() (C)
 *
 * Assumptions:
 *  - CPU is in SVC mode
 *  - IRQ/FIQ are masked
 *  - MMU and cache are OFF
 *  - ALL exception mode stacks already setup in reset.S
 *
 * NOTE: Stack setup has been moved to reset.S to avoid
 * "death zone" where exceptions could occur before stacks
 * are ready. See reset.S for stack initialization.
 * ============================================================
 */

    .section .text.boot_entry, "ax"
    .align  4
    .global entry

entry:
    /* --------------------------------------------------------
     * Clear .bss section
     * --------------------------------------------------------
     * r0 = start of .bss
     * r1 = end of .bss
     * r2 = zero
     */
    ldr     r0, =_bss_start
    ldr     r1, =_bss_end
    mov     r2, #0

1:
    cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b

    /* --------------------------------------------------------
     * Enter C kernel
     * --------------------------------------------------------
     * From this point, boot phase ends.
     */
    bl      kernel_main

    /* --------------------------------------------------------
     * Should never return
     * --------------------------------------------------------
     */
halt:
    b       halt

/* ------------------------------------------------------------
 * External symbols from linker
 * ------------------------------------------------------------
 */
    .extern _bss_start
    .extern _bss_end
    .extern kernel_main