/* ============================================================
 * reset.S
 * ------------------------------------------------------------
 * Reset handler — first code after CPU reset
 * ============================================================
 * - MMU: OFF
 * - Cache: OFF
 * - No stack initially
 *
 * IMPORTANT (3G/1G split):
 *   Stack symbols (_svc_stack_top etc.) are at kernel VA
 *   (0xC0xxxxxx) because the linker places .stack in the
 *   kernel VA region. At boot time MMU is OFF, so we must
 *   convert VA → PA by subtracting VA_OFFSET (0x40000000).
 *   After the MMU trampoline in entry.S, stack pointers are
 *   reloaded to the correct VA values.
 * ============================================================ */

    .section .text.reset, "ax"
    .align 4
    .global reset_handler

/* VA_OFFSET = KERNEL_VA_BASE - DDR_PA_BASE = 0x40000000
 * This is a valid ARM immediate: 0x01 ROR 2 */
.equ VA_OFFSET_BOOT, 0x40000000

reset_handler:
    /* --------------------------------------------------------
     * Mask IRQ and FIQ
     * -------------------------------------------------------- */
    cpsid   if

    /* --------------------------------------------------------
     * Ensure CPU is in SVC mode
     * -------------------------------------------------------- */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0

    /* --------------------------------------------------------
     * Setup stacks for ALL exception modes
     * --------------------------------------------------------
     * CRITICAL: Must setup ALL exception stacks BEFORE
     * configuring VBAR, because once VBAR is set, any
     * exception will use these stacks!
     *
     * Linker symbols are VA (0xC0xxxxxx). Subtract VA_OFFSET
     * to get PA for pre-MMU execution.
     * -------------------------------------------------------- */

    /* FIQ mode stack */
    cps     #0x11
    ldr     sp, =_fiq_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    /* IRQ mode stack */
    cps     #0x12
    ldr     sp, =_irq_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    /* ABT mode stack (for Data/Prefetch Abort) */
    cps     #0x17
    ldr     sp, =_abt_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    /* UND mode stack (for Undefined Instruction) */
    cps     #0x1B
    ldr     sp, =_und_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    /* Return to SVC mode and setup SVC stack */
    cps     #0x13
    ldr     sp, =_svc_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    /* --------------------------------------------------------
     * Set VBAR to boot-time vector table (PA)
     * --------------------------------------------------------
     * _boot_start = PA 0x80000000 (in .text.boot, VMA == PA)
     * This is correct for MMU-off execution.
     * After the MMU trampoline, VBAR is updated to VA.
     * -------------------------------------------------------- */
    ldr     r0, =_boot_start
    mcr     p15, 0, r0, c12, c0, 0     /* Write VBAR */
    isb

    /* --------------------------------------------------------
     * Jump to entry.S
     * -------------------------------------------------------- */
    b       entry

    .extern _boot_start
    .extern _fiq_stack_top
    .extern _irq_stack_top
    .extern _abt_stack_top
    .extern _und_stack_top
    .extern _svc_stack_top
    .extern entry