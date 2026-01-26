/* ============================================================
 * reset.S
 * ------------------------------------------------------------
 * Reset handler for AM335x / Cortex-A8
 * - First code executed after CPU reset
 * - MMU: OFF
 * - Cache: OFF
 * - No stack initially
 * ============================================================
 */

    .section .vectors, "ax"
    .align 5
    .global _start

/* ------------------------------------------------------------
 * Vector Table (Low vectors)
 * ------------------------------------------------------------
 * Each entry branches to its handler.
 * Only reset handler is implemented here.
 */
_start:
    b   reset_handler           /* Reset */
    b   .                       /* Undefined Instruction */
    b   .                       /* SVC */
    b   .                       /* Prefetch Abort */
    b   .                       /* Data Abort */
    b   .                       /* Reserved */
    b   .                       /* IRQ */
    b   .                       /* FIQ */

/* ------------------------------------------------------------
 * Reset Handler
 * ------------------------------------------------------------
 */
    .section .text.reset, "ax"
    .align 4
    .global reset_handler

reset_handler:
    /* --------------------------------------------------------
     * Mask IRQ and FIQ
     * --------------------------------------------------------
     */
    cpsid   if

    /* --------------------------------------------------------
     * Ensure CPU is in SVC mode
     * --------------------------------------------------------
     * CPSR[4:0] = 10011 (SVC)
     */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0

    /* --------------------------------------------------------
     * Setup initial stack for SVC mode
     * Stack address provided by linker
     * --------------------------------------------------------
     */
    ldr     r0, =_svc_stack_top
    mov     sp, r0

    /* --------------------------------------------------------
     * Set Vector Base Address Register (VBAR)
     * --------------------------------------------------------
     * Point VBAR to start of vector table
     */
    ldr     r0, =_vector_start
    mcr     p15, 0, r0, c12, c0, 0   /* Write VBAR */
    isb                              /* Instruction barrier */

    /* --------------------------------------------------------
     * Jump to entry.S for further initialization
     * --------------------------------------------------------
     */
    b       entry

/* ------------------------------------------------------------
 * External symbols from linker
 * ------------------------------------------------------------
 */
    .extern _vector_start
    .extern _svc_stack_top
    .extern entry