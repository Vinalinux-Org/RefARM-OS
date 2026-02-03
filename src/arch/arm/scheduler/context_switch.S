/* ============================================================
 * context_switch.S
 * ------------------------------------------------------------
 * Context switching implementation for RefARM-OS scheduler
 * Target: ARMv7-A (Cortex-A8) / BeagleBone Black
 * 
 * CRITICAL implementation based on:
 * - ARM Architecture Reference Manual ARMv7-A Part B (Exception Handling)
 * - docs/05_scheduler/03_context_switch_contract.md (corrected version)
 * ============================================================ */

.global context_switch
.global start_first_task

.section .text

/* ============================================================
 * context_switch - Switch from current task to next task
 * ============================================================
 * 
 * Called from timer ISR (IRQ mode)
 * 
 * Arguments:
 *   r0 = struct task_struct *current (pointer to current task)
 *   r1 = struct task_struct *next (pointer to next task)
 * 
 * Stack frame layout (60 bytes, low → high address):
 *   [SPSR:4] [LR:4] [r0-r12:52] ← SP
 * 
 * Contract:
 * - Must save ALL general purpose registers
 * - Must save LR (return address)
 * - Must save SPSR (task's CPSR snapshot)
 * - Must switch SP for new task
 * - Must NOT corrupt any registers of either task
 * 
 * CRITICAL NOTES:
 * - We are in IRQ mode when called
 * - SPSR_irq contains task's CPSR (set by hardware on IRQ entry)
 * - We switch to SVC mode to access task's stack
 * - We use SPSR, NOT CPSR (common mistake!)
 * - Memory barrier (DSB) ensures stack writes complete
 */
context_switch:
    /* ========================================
     * SAVE CURRENT TASK
     * ======================================== */
    
    /* 
     * Step 1: Switch to SVC mode to access task's banked registers
     * 
     * Use MSR instead of CPS for safety in IRQ context
     * 0xD3 = 0b11010011:
     *   - Mode bits [4:0] = 0x13 (SVC)
     *   - I bit [7] = 1 (IRQ disabled)
     *   - F bit [6] = 1 (FIQ disabled) 
     */
    msr     cpsr_c, #0xD3           @ Switch to SVC mode, IRQ/FIQ disabled
    
    /* Now in SVC mode - can access r13_svc and r14_svc */
    
    /* 
     * Step 2: Save general purpose registers on task's stack
     * STMFD = Store Multiple Full Descending (push)
     * sp! means decrement SP during store
     */
    stmfd   sp!, {r0-r12}           @ Push r0-r12 (52 bytes)
    
    /* 
     * Step 3: Save task's link register
     * LR contains where task was executing
     */
    stmfd   sp!, {lr}               @ Push r14_svc (4 bytes)
    
    /* 
     * Step 4: Get and save SPSR_irq
     * 
     * CRITICAL: SPSR_irq is still accessible even in SVC mode
     * because we haven't returned from IRQ exception yet.
     * It contains task's original CPSR before interrupt.
     * 
     * MRS spsr reads SPSR of the mode we CAME FROM (IRQ mode)
     */
    mrs     r2, spsr                @ Read SPSR_irq into r2
    stmfd   sp!, {r2}               @ Push SPSR (4 bytes)
    
    /* Stack frame complete: [SPSR] [LR] [r0-r12], SP at bottom */
    
    /* 
     * Step 5: Save current task's SP
     * 
     * r0 still contains current task pointer
     * task_struct.context is at offset 0
     * task_context.sp is at offset 52 (after r0-r12)
     * 
     * Since struct task_context starts at offset 0 of task_struct,
     * and sp is the 14th field (after r0-r12), offset = 13*4 = 52
     */
    str     sp, [r0, #52]           @ current->context.sp = sp
    
    /* 
     * Step 6: Memory barrier
     * 
     * Ensure all stack writes complete before loading next task.
     * Critical on Cortex-A8 due to write buffer.
     */
    dsb                             @ Data Synchronization Barrier
    
    /* ========================================
     * LOAD NEXT TASK
     * ======================================== */
    
    /* 
     * Step 7: Load next task's SP
     * r1 contains next task pointer
     */
    ldr     sp, [r1, #52]           @ sp = next->context.sp
    
    /* 
     * Step 8: Restore next task's SPSR
     * 
     * Pop SPSR from stack and write to SPSR register.
     * This will be the task's CPSR when we return.
     * 
     * msr spsr_cxsf writes all fields:
     *   c = control (mode, interrupt bits)
     *   x = extension
     *   s = status (condition flags)
     *   f = flags
     */
    ldmfd   sp!, {r2}               @ Pop SPSR into r2
    msr     spsr_cxsf, r2           @ Write to SPSR
    
    /* 
     * Step 9: Restore next task's LR
     */
    ldmfd   sp!, {lr}               @ Pop LR into r14_svc
    
    /* 
     * Step 10: Restore next task's GPRs
     */
    ldmfd   sp!, {r0-r12}           @ Pop r0-r12
    
    /* 
     * Step 11: Return to task
     * 
     * MOVS (S suffix) is a privileged return:
     * - PC ← LR (jump to task)
     * - CPSR ← SPSR (restore processor state)
     * 
     * This is atomic: both operations happen together.
     * Task resumes with restored registers and CPSR.
     */
    movs    pc, lr                  @ Privileged return

/* ============================================================
 * start_first_task - Initialize first task execution
 * ============================================================
 * 
 * Called once at scheduler startup to load first task.
 * Does NOT save any context (no current task exists).
 * 
 * Arguments:
 *   r0 = struct task_struct *first (pointer to first task)
 * 
 * Must:
 * - Load task's initial stack frame
 * - Restore all registers from frame
 * - Jump to task entry point
 * - Set CPSR to SVC mode with IRQ enabled
 */
start_first_task:
    /* 
     * Step 1: Switch to SVC mode
     * Same as context_switch
     */
    msr     cpsr_c, #0xD3           @ SVC mode, IRQ/FIQ disabled
    
    /* 
     * Step 2: Load task's SP
     */
    ldr     sp, [r0, #52]           @ sp = first->context.sp
    
    /* 
     * Step 3: Pop and set SPSR
     * For first task, this contains SVC mode + IRQ enabled
     */
    ldmfd   sp!, {r2}               @ Pop initial SPSR
    msr     spsr_cxsf, r2           @ Set SPSR
    
    /* 
     * Step 4: Pop LR (entry point)
     */
    ldmfd   sp!, {lr}               @ Pop entry point into LR
    
    /* 
     * Step 5: Pop GPRs (all zeros for new task)
     */
    ldmfd   sp!, {r0-r12}           @ Pop r0-r12
    
    /* 
     * Step 6: Jump to task with CPSR restore
     * MOVS makes CPSR ← SPSR, enabling IRQ as configured
     */
    movs    pc, lr                  @ Jump to entry point, restore CPSR

.end
