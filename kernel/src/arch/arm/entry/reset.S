/* ============================================================
 * reset.S
 * ------------------------------------------------------------
 * Reset handler - first code executed after CPU reset
 * ============================================================
 * - MMU: OFF
 * - Cache: OFF
 * - No stack initially
 * ============================================================
 */

/* ============================================================
 * NOTE: Vector table has been moved to exceptions/vectors.S
 * This file now contains ONLY the reset handler.
 * ============================================================ */

    .section .text.reset, "ax"
    .align 4
    .global reset_handler

/* ------------------------------------------------------------
 * Reset Handler
 * ------------------------------------------------------------
 * Executed when CPU resets or branches from vector table.
 * 
 * Responsibilities:
 *   1. Mask IRQ/FIQ
 *   2. Ensure SVC mode
 *   3. Setup SVC stack
 *   4. Configure VBAR to point to vector table
 *   5. Jump to entry.S for BSS clear and kernel_main()
 * ------------------------------------------------------------
 */
reset_handler:
    /* --------------------------------------------------------
     * Mask IRQ and FIQ
     * --------------------------------------------------------
     * Ensure no interrupts can occur during boot.
     */
    cpsid   if

    /* --------------------------------------------------------
     * Ensure CPU is in SVC mode
     * --------------------------------------------------------
     * CPSR[4:0] = 10011 (SVC)
     * This is defensive - should already be in SVC after reset.
     */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0

    /* --------------------------------------------------------
     * Setup stacks for ALL exception modes
     * --------------------------------------------------------
     * CRITICAL: Must setup ALL exception stacks BEFORE
     * configuring VBAR, because once VBAR is set, any
     * exception (even accidental) will use these stacks!
     * 
     * This is the ONLY safe place to setup stacks - doing
     * it later creates a "death zone" where exceptions
     * would crash due to invalid stack pointers.
     * -------------------------------------------------------- */

    /* FIQ mode stack */
    cps     #0x11                   /* Enter FIQ mode */
    ldr     sp, =_fiq_stack_top
    
    /* IRQ mode stack */
    cps     #0x12                   /* Enter IRQ mode */
    ldr     sp, =_irq_stack_top
    
    /* ABT mode stack (for Data/Prefetch Abort) */
    cps     #0x17                   /* Enter ABT mode */
    ldr     sp, =_abt_stack_top
    
    /* UND mode stack (for Undefined Instruction) */
    cps     #0x1B                   /* Enter UND mode */
    ldr     sp, =_und_stack_top
    
    /* Return to SVC mode and setup SVC stack */
    cps     #0x13                   /* Enter SVC mode */
    ldr     sp, =_svc_stack_top

    /* --------------------------------------------------------
     * Set Vector Base Address Register (VBAR)
     * --------------------------------------------------------
     * Point VBAR to start of vector table (from vectors.S)
     * _vector_start is defined by linker (0x80000000)
     * 
     * CRITICAL: ISB instruction is required after writing VBAR
     * to ensure the write completes before any exceptions occur.
     * 
     * NOTE: Stack setup MUST be done BEFORE this point!
     * Once VBAR is set, exception handlers expect valid stacks.
     */
    ldr     r0, =_vector_start
    mcr     p15, 0, r0, c12, c0, 0   /* Write VBAR */
    isb                              /* Instruction synchronization barrier */

    /* --------------------------------------------------------
     * Jump to entry.S for further initialization
     * --------------------------------------------------------
     * entry.S will:
     *   - Clear BSS
     *   - Call kernel_main()
     */
    b       entry

/* ------------------------------------------------------------
 * External symbols from linker
 * ------------------------------------------------------------
 */
    .extern _vector_start
    .extern _fiq_stack_top
    .extern _irq_stack_top
    .extern _abt_stack_top
    .extern _und_stack_top
    .extern _svc_stack_top
    .extern entry