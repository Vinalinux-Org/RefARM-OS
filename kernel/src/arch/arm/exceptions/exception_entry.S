/* ============================================================
 * exception_entry.S
 * ------------------------------------------------------------
 * Exception Entry Stubs
 * Handles state saving/restoring and mode switching for all
 * ARM exceptions (IRQ, SVC, Abort, Undefined).
 * ============================================================ */

/* ============================================================
 * Exception Entry Conventions
 * ------------------------------------------------------------
 * Each exception type enters in a different CPU mode:
 *   - Undefined:      UND mode
 *   - SVC:            SVC mode
 *   - Prefetch Abort: ABT mode
 *   - Data Abort:     ABT mode
 *   - IRQ:            IRQ mode
 *   - FIQ:            FIQ mode
 * 
 * Each stub must:
 *   1. Adjust LR (return address) if needed
 *   2. Save full context (SPSR + r0-r12 + LR)
 *   3. Switch to SVC mode (unified handling)
 *   4. Call C handler (pass context pointer in r0)
 *   5. Halt (no return - fatal exceptions)
 * 
 * Context layout (pushed to stack):
 *   [High Address]
 *   LR            <- Pushed first (by stmfd {r0-r12, lr})
 *   r12
 *   ...
 *   r0
 *   SPSR          <- Pushed last (by stmfd {r0}), SP points here
 *   [Low Address]
 * ============================================================ */

.global exception_entry_undefined
.global exception_entry_svc
.global exception_entry_prefetch_abort
.global exception_entry_data_abort
.global exception_entry_irq
.global exception_entry_fiq

/* ============================================================
 * Undefined Instruction Exception Entry
 * ------------------------------------------------------------
 * Entered in: UND mode
 * LR_und = address of undefined instruction + 4
 * SPSR_und = CPSR before exception
 * ============================================================ */
exception_entry_undefined:
    /* Adjust return address (LR points to instruction AFTER the undefined one) */
    sub     lr, lr, #4
    
    /* Save context on exception mode stack (UND mode stack)
     * CRITICAL ORDER:
     * 1. Save r0-r12, lr FIRST (preserve original r0!)
     * 2. Get stack pointer (while still in UND mode)
     * 3. Then switch mode
     */
    stmfd   sp!, {r0-r12, lr}
    
    /* Save SPSR after general registers */
    mrs     r0, spsr
    stmfd   sp!, {r0}
    
    /* Get context pointer (still on UND stack) */
    mov     r0, sp
    
    /* Switch to SVC mode (keep interrupts masked) */
    cps     #0x13
    
    /* Call C handler (pass context pointer from r0) */
    bl      handle_undefined_instruction
    
    /* Should never return - halt */
    b       .

/* ============================================================
 * Supervisor Call (SVC) Exception Entry
 * ------------------------------------------------------------
 * Entered in: SVC mode (already in supervisor)
 * LR_svc = address of instruction AFTER the SVC
 * SPSR_svc = CPSR before exception
 * ============================================================ */
exception_entry_svc:
    /* ==========================================================
     * SVC Handler (User Mode Entry/Syscall)
     * ----------------------------------------------------------
     * Entered in: SVC mode
     * LR_svc: Points to instruction AFTER "svc #n"
     * SPSR_svc: Saved CPSR of the caller (User Mode)
     * ========================================================== */

    /* 1. Save General Purpose Registers + LR (14 registers) */
    /* Stack check: 14 regs * 4 bytes = 56 bytes */
    stmfd   sp!, {r0-r12, lr}
    
    /* 2. Save SPSR (1 register) WITH ALIGNMENT PADDING 
     * AAPCS requires SP to be 8-byte aligned at public interfaces.
     * Currently pushes: 14 regs. Need 2 more to make 16 (16*4=64).
     * We push {SPSR, Padding}.
     */
    mrs     r0, spsr
    mov     r1, #0              /* Zero padding */
    stmfd   sp!, {r0, r1}       /* SP -= 8. [SP]=SPSR, [SP+4]=Pad */
    
    /* 3. Re-enable IRQ during syscall processing.
     * ARM hardware sets I-bit=1 on SVC entry, disabling IRQ.
     * We must re-enable here so timer ticks and UART RX interrupts
     * continue to fire. Without this, context_switch from the SVC
     * handler bleeds IRQ-disabled state into the next task (idle),
     * permanently killing all interrupts.
     * On return, ldmfd {pc}^ atomically restores CPSR from SPSR,
     * so the caller's original IRQ state is safely recovered. */
    cpsie   i
    
    /* 4. Prepare C Handler Arguments */
    mov     r0, sp              /* r0 = struct pt_regs *regs */
    
    /* 5. Call C Handler */
    bl      svc_handler         /* src/kernel/core/svc_handler.c */
    
    /* 5. Restore SPSR */
    .global svc_exit_trampoline
svc_exit_trampoline:
    /* 
     * Stack at entry: [Pad, SPSR, R0..R12, PC] 
     * SP points to Pad/SPSR (Low Address)
     */

    /* 1. Pop Padding and SPSR */
    ldmfd   sp!, {r0, r1}      /* R0=SPSR, R1=Pad */
    msr     spsr_cxsf, r0
    
    /* 
     * 2. Restore Context and Return 
     * Note: SP_usr was already set by start_first_task or context_switch
     */
    
    /* 3. Restore Context and Return 
     * "^" suffix copies SPSR_svc -> CPSR (Mode Switch)
     */
    ldmfd   sp!, {r0-r12, pc}^

/* ============================================================
 * Prefetch Abort Exception Entry
 * ------------------------------------------------------------
 * Entered in: ABT mode
 * LR_abt = address of aborted instruction + 4
 * SPSR_abt = CPSR before exception
 * ============================================================ */
exception_entry_prefetch_abort:
    /* Adjust return address (LR points 4 bytes ahead) */
    sub     lr, lr, #4
    
    /* Save context on ABT mode stack
     * Order: r0-r12, lr first, then SPSR
     */
    stmfd   sp!, {r0-r12, lr}
    
    /* Save SPSR after general registers */
    mrs     r0, spsr
    stmfd   sp!, {r0}
    
    /* Get context pointer (still on ABT stack) */
    mov     r0, sp
    
    /* Switch to SVC mode (keep interrupts masked) */
    cps     #0x13
    
    /* Call C handler (pass context pointer) */
    bl      handle_prefetch_abort
    
    /* Should never return - halt */
    b       .

/* ============================================================
 * Data Abort Exception Entry
 * ------------------------------------------------------------
 * Entered in: ABT mode
 * LR_abt = address of aborted instruction + 8
 * SPSR_abt = CPSR before exception
 * ============================================================ */
exception_entry_data_abort:
    /* Adjust return address (LR points 8 bytes ahead) */
    sub     lr, lr, #8
    
    /* Save context on ABT mode stack
     * Order: r0-r12, lr first, then SPSR
     */
    stmfd   sp!, {r0-r12, lr}
    
    /* Save SPSR after general registers */
    mrs     r0, spsr
    stmfd   sp!, {r0}
    
    /* Get context pointer (still on ABT stack) */
    mov     r0, sp
    
    /* Switch to SVC mode (keep interrupts masked) */
    cps     #0x13
    
    /* Call C handler (pass context pointer) */
    bl      handle_data_abort
    
    /* Should never return - halt */
    b       .

/* ============================================================
 * IRQ Exception Entry
 * ------------------------------------------------------------
 * Entered in: IRQ mode
 * LR_irq = address of next instruction + 4
 * SPSR_irq = CPSR before exception
 * 
 * This stub:
 * 1. Adjusts LR to point to interrupted instruction
 * 2. Saves context (r0-r12, lr, SPSR)
 * 3. Calls irq_dispatch() in C
 * 4. Restores context
 * 5. Returns from exception
 * ============================================================ */
/* ============================================================
 * IRQ Exception Entry (SRS/RFE Model)
 * ------------------------------------------------------------
 * Entered in: IRQ mode
 * LR_irq = address of next instruction + 4
 * SPSR_irq = CPSR before exception
 * 
 * This stub:
 * 1. Adjusts LR (LR-4)
 * 2. Saves LR, SPSR to SVC stack using SRSDB
 * 3. Switches to SVC mode
 * 4. Saves GPRs to SVC stack
 * 5. Calls handler
 * 6. Restores GPRs
 * 7. Returns using RFE (from SVC stack)
 * ============================================================ */
exception_entry_irq:
    /*
     * CRITICAL: Adjust return address FIRST (PC+4 -> PC)
     */
    sub     lr, lr, #4
    
    /*
     * 1. Save Return State (LR_irq, SPSR_irq) to SVC Stack
     * This moves context safety to the Thread's Kernel Stack immediately.
     * Mode 0x13 = SVC.
     */
    srsdb   sp!, #0x13
    
    /*
     * 2. Switch to SVC Mode
     * We serve the IRQ in Supervisor Mode (standard for simple OS)
     */
    cps     #0x13
    
    /*
     * 3. Save General Purpose Registers + LR_svc
     * We push LR_svc to maintain 8-byte alignment (14 regs total).
     * Note: LR_svc is likely scratch if we came from User Mode, 
     * but we save it to allow kernel preemption later if needed.
     */
    stmfd   sp!, {r0-r12, lr}
    
    /*
     * 4. Call C dispatcher
     * r0 = pointer to stack (struct exception_context*)
     * Note: Struct layout mismatch vs Stack:
     * Stack: [R0..R12, LR_svc, SPSR_irq, LR_irq]
     * Struct: [SPSR, R0..R12, LR] (Old layout)
     * irq_dispatch doesn't use ctx, so this is safe for now.
     */
    mov     r0, sp
    bl      irq_dispatch
    
    /*
     * 5. Restore Context
     */
    ldmfd   sp!, {r0-r12, lr}
    
    /*
     * 6. Return from Exception (RFE)
     * Loads PC and CPSR from the stack (pushed by srsdb).
     * Automatically handles mode switch back to User/Previous mode.
     * "^" suffix not needed (RFE handles SPSR->CPSR).
     */
    rfefd   sp!

/* ============================================================
 * FIQ Exception Entry (Stub)
 * ------------------------------------------------------------
 * Entered in: FIQ mode
 * LR_fiq = address of next instruction + 4
 * SPSR_fiq = CPSR before exception
 * 
 * NOTE: FIQ is NOT USED in this project. If this executes,
 * it indicates a serious bug.
 * ============================================================ */
exception_entry_fiq:
    /* Adjust return address */
    sub     lr, lr, #4
    
    /* Save context on FIQ mode stack
     * Order: r0-r12, lr first, then SPSR
     */
    stmfd   sp!, {r0-r12, lr}
    
    /* Save SPSR after general registers */
    mrs     r0, spsr
    stmfd   sp!, {r0}
    
    /* Get context pointer (still on FIQ stack) */
    mov     r0, sp
    
    /* Switch to SVC mode */
    cps     #0x13
    
    /* Call C handler (should print error and halt) */
    bl      handle_fiq
    
    /* Should never return - halt */
    b       .

/* ============================================================
 * Notes on LR Adjustment
 * ------------------------------------------------------------
 * ARM exceptions save PC+offset in LR depending on exception type:
 *   - Undefined:      PC+4 (next instruction)
 *   - SVC:            PC+4 (next instruction)
 *   - Prefetch Abort: PC+4 (faulting instruction + 4)
 *   - Data Abort:     PC+8 (faulting instruction + 8)
 *   - IRQ:            PC+4 (next instruction)
 *   - FIQ:            PC+4 (next instruction)
 * 
 * To get the actual faulting PC, we adjust:
 *   - Undefined:      LR-4 → faulting instruction
 *   - SVC:            LR-0 → instruction after SVC
 *   - Prefetch Abort: LR-4 → faulting instruction
 *   - Data Abort:     LR-8 → faulting instruction
 *   - IRQ/FIQ:        LR-4 → interrupted instruction
 * 
 * Reference: ARM Architecture Reference Manual
 * ============================================================ */