/* ============================================================
 * exception_entry.S
 * ------------------------------------------------------------
 * Exception entry stubs for ARM exceptions
 * Architecture: ARMv7-A (Cortex-A8)
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
 *   SPSR          <- First pushed
 *   r0
 *   r1
 *   ...
 *   r12
 *   LR            <- Last pushed, SP points here
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
    /* No LR adjustment needed (already points to next instruction) */
    
    /* Save context on SVC mode stack
     * Order: r0-r12, lr first, then SPSR
     */
    stmfd   sp!, {r0-r12, lr}
    
    /* Save SPSR after general registers */
    mrs     r0, spsr
    stmfd   sp!, {r0}
    
    /* Get context pointer (already on SVC stack) */
    mov     r0, sp
    
    /* Already in SVC mode - no switch needed */
    
    /* Call C handler (pass context pointer) */
    bl      handle_svc
    
    /* Should never return - halt */
    b       .

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
 * IRQ Exception Entry (Stub)
 * ------------------------------------------------------------
 * Entered in: IRQ mode
 * LR_irq = address of next instruction + 4
 * SPSR_irq = CPSR before exception
 * 
 * NOTE: IRQ is MASKED in current phase, so this should never
 * execute. If it does, it indicates a bug (IRQ enabled when
 * it shouldn't be).
 * ============================================================ */
exception_entry_irq:
    /* Adjust return address */
    sub     lr, lr, #4
    
    /* Save context on IRQ mode stack
     * Order: r0-r12, lr first, then SPSR
     */
    stmfd   sp!, {r0-r12, lr}
    
    /* Save SPSR after general registers */
    mrs     r0, spsr
    stmfd   sp!, {r0}
    
    /* Get context pointer (still on IRQ stack) */
    mov     r0, sp
    
    /* Switch to SVC mode */
    cps     #0x13
    
    /* Call C handler (should print error and halt) */
    bl      handle_irq
    
    /* Should never return - halt */
    b       .

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