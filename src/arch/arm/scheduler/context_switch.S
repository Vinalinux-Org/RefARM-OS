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
 * Standard Callee-Saved Context Switch (Cooperative)
 * 
 * Arguments:
 *   r0 = struct task_struct *current (pointer to current task)
 *   r1 = struct task_struct *next (pointer to next task)
 * 
 * Stack frame layout (36 bytes):
 *   [LR] [R11] [R10] ... [R4]
 * 
 * Note: R0-R3, R12 are caller-saved and handled by C compiler if needed.
 *       SPSR is NOT saved because we are staying in Kernel Mode (SVC).
 */
context_switch:
    /* 
     * 1. Save Callee-Saved Registers + LR
     * stmfd sp!, {r4-r11, lr}
     */
    stmfd   sp!, {r4-r11, lr}
    
    /* 
     * 2. Save Stack Pointer to current task structure
     * current->context.sp is at offset 52 (13 * 4) 
     * defined in existing task_struct layout matching r0-r12 placeholder
     */
    str     sp, [r0, #52]
    
    /* 
     * 3. Memory barrier
     */
    dsb
    
    /* ========================================
     * LOAD NEXT TASK
     * ======================================== */
    
    /* 
     * 4. Load next task's SP
     * r1 contains next task pointer
     */
    ldr     sp, [r1, #52]
    
    /* 
     * 5. Restore next task's Registers
     */
    ldmfd   sp!, {r4-r11, lr}
    
    /* 
     * 6. Return to task (jump to LR)
     * BX LR allows interworking (Thumb/ARM), though we are likely ARM-only.
     * This returns to the instruction after context_switch() call.
     */
    bx      lr

/* ============================================================
 * start_first_task - Initialize first task execution
 * ============================================================
 * 
 * Called once at scheduler startup to load first task.
 * Does NOT save any context (no current task exists).
 * 
 * Arguments:
 *   r0 = struct task_struct *first (pointer to first task)
 */
start_first_task:
    /* 
     * 1. Load task's SP
     */
    ldr     sp, [r0, #52]
    
    /* 
     * 2. Restore Registers (pop Frame 2: Kernel Context)
     */
    ldmfd   sp!, {r4-r11, lr}
    
    /* 
     * 3. Jump to task (actually jumps to svc_exit_trampoline)
     */
    bx      lr

.end
