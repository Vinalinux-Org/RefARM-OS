/* ============================================================
 * start.S
 * Bootloader entry point
 * 
 * ARM Cortex-A8 Vector Table and CPU Mode Setup
 * ============================================================ */

    .section .vectors, "ax"
    .align 5      /* Align to 32-byte boundary for vector table */
    .global _vectors

_vectors:
    ldr     pc, =_start          /* Reset vector */
    ldr     pc, =_undef_handler  /* Undefined Instruction */
    ldr     pc, =_svc_handler    /* Supervisor Call (SVC) */
    ldr     pc, =_prefetch_handler /* Prefetch Abort */
    ldr     pc, =_data_handler   /* Data Abort */
    ldr     pc, =_reserved_handler /* Reserved */
    ldr     pc, =_irq_handler    /* IRQ */
    ldr     pc, =_fiq_handler    /* FIQ */

    .section .text.start, "ax"
    .align 4
    .global _start

_start:
    /* ========================================================
     * CPU Initialization Sequence
     * ======================================================== */
    
    /* 1. Mask all interrupts */
    cpsid   if

    /* 2. Setup stacks for ALL exception modes BEFORE any exceptions can occur
     *    Order matters: Must setup stacks before VBAR is set
     *    According to ARM ARM: "If an exception occurs when the processor
     *    is using a stack pointer that is not initialized, the system will crash"
     */
    
    /* FIQ mode (0x11) */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x11
    msr     cpsr_c, r0
    ldr     sp, =_fiq_stack_top
    
    /* IRQ mode (0x12) */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x12
    msr     cpsr_c, r0
    ldr     sp, =_irq_stack_top
    
    /* Abort mode (0x17) */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x17
    msr     cpsr_c, r0
    ldr     sp, =_abt_stack_top
    
    /* Undefined mode (0x1B) */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x1B
    msr     cpsr_c, r0
    ldr     sp, =_und_stack_top
    
    /* Supervisor mode (0x13) - main bootloader mode */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0
    ldr     sp, =_svc_stack_top

    /* System mode (0x1F) - same as user but privileged */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x1F
    msr     cpsr_c, r0
    ldr     sp, =_sys_stack_top

    /* Disable WDT1 (TRM 26.1.5.1) */
    ldr     r0, =0x44E35000   @ WDT1 base
    ldr     r1, =0xAAAA
    str     r1, [r0, #0x48]   @ WDT_WSPR
1:  ldr     r2, [r0, #0x34]   @ Wait WDT_WWPS.W_PEND_WSPR
    tst     r2, #(1 << 4)
    bne     1b
    ldr     r1, =0x5555
    str     r1, [r0, #0x48]

    /* Return to Supervisor mode for bootloader */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0

    /* 3. Setup Vector Base Address Register (VBAR)
     *    Must be done AFTER stacks are setup
     */
    ldr     r0, =_vectors
    mcr     p15, 0, r0, c12, c0, 0   /* Write VBAR */
    isb                              /* Instruction synchronization barrier */

    /* 4. Clear BSS section */
    ldr     r0, =_bss_start
    ldr     r1, =_bss_end
    mov     r2, #0

1:
    cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b

    /* 5. Jump to C bootloader_main */
    bl      bootloader_main

    /* 6. If bootloader_main returns, hang */
halt:
    b       halt

/* ============================================================
 * Minimal Exception Handlers
 *
 * ARMv7-A ABI Rule: C functions destroy r0-r3, r12, LR.
 * When entering an exception, ARM saves the PREVIOUS mode's PC to
 * the exception mode's LR (banked). We must save r0-r3, r12, and
 * the banked LR onto the exception stack before calling any C function.
 *
 * Stack frame layout (grows down, ARM AAPCS):
 *   Old SP → [LR_exc] [r12] [r3] [r2] [r1] [r0] ← SP after push
 * ============================================================ */

    .section .text.handlers, "ax"
    .align 4

_undef_handler:
    /* Undefined Instruction exception — processor is now in UNDEFINED mode (0x1B)
     * LR = address of instruction AFTER the undefined one
     * Save r0-r3, r12, LR before calling C (AAPCS scratch registers) */
    push    {r0-r3, r12, lr}
    bl      c_undef_handler
    pop     {r0-r3, r12, lr}
    movs    pc, lr              /* Return from exception (restores CPSR from SPSR) */
1:  b       1b                  /* Should never reach here — c_undef_handler loops */

_svc_handler:
    /* SVC — not used by bootloader, hang */
    b       _svc_handler

_prefetch_handler:
    /* Prefetch Abort — processor in ABORT mode (0x17)
     * LR = address of instruction that CAUSED the fault + 4
     * Save r0-r3, r12, LR before calling C */
    push    {r0-r3, r12, lr}
    bl      c_prefetch_abort_handler
    pop     {r0-r3, r12, lr}
    subs    pc, lr, #4          /* Return to faulting instruction (retry) */
1:  b       1b

_data_handler:
    /* Data Abort — processor in ABORT mode (0x17)
     * LR = address of instruction that CAUSED the fault + 8
     * Save r0-r3, r12, LR before calling C */
    push    {r0-r3, r12, lr}
    bl      c_data_abort_handler
    pop     {r0-r3, r12, lr}
    subs    pc, lr, #8          /* Return to faulting instruction */
1:  b       1b

_reserved_handler:
    b       _reserved_handler

_irq_handler:
    /* IRQ — not enabled in bootloader, hang */
    b       _irq_handler

_fiq_handler:
    /* FIQ — not enabled in bootloader, hang */
    b       _fiq_handler

/* ============================================================
 * External symbols
 * ============================================================ */
    .extern _bss_start
    .extern _bss_end
    .extern _svc_stack_top
    .extern _irq_stack_top
    .extern _fiq_stack_top
    .extern _abt_stack_top
    .extern _und_stack_top
    .extern _sys_stack_top
    .extern bootloader_main
    .extern c_undef_handler
    .extern c_prefetch_abort_handler
    .extern c_data_abort_handler
