/* ============================================================
 * entry.S
 * ------------------------------------------------------------
 * Entry point after reset handler.
 *
 * Responsibilities:
 *  - Setup stack for IRQ mode (prepare for future interrupt use)
 *  - Clear .bss section
 *  - Transfer control to kernel_main() (C)
 *
 * Assumptions:
 *  - CPU is in SVC mode
 *  - IRQ/FIQ are masked
 *  - MMU and cache are OFF
 * ============================================================
 */

    .section .text.entry, "ax"
    .align  4
    .global entry

entry:
    /* --------------------------------------------------------
     * Setup IRQ mode stack
     * --------------------------------------------------------
     * Switch to IRQ mode, set SP, then return to SVC mode.
     * IRQ remains masked.
     */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x12          /* IRQ mode */
    msr     cpsr_c, r0

    ldr     sp, =_irq_stack_top

    /* --------------------------------------------------------
     * Return to SVC mode
     * --------------------------------------------------------
     */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13          /* SVC mode */
    msr     cpsr_c, r0

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
 * External symbols
 * ------------------------------------------------------------
 */
    .extern _irq_stack_top
    .extern _bss_start
    .extern _bss_end
    .extern kernel_main
