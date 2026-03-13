/* ============================================================
 * vectors.S
 * ------------------------------------------------------------
 * ARM Exception Vector Table
 * ============================================================
 *
 * LDR PC pattern — loads full 32-bit handler address from a
 * literal pool placed immediately after the 8 vector slots.
 * This allows handlers to be anywhere in the 4GB address space,
 * which is required because exception_entry stubs live at
 * kernel VA (0xC0xxxxxx) while this table is at PA 0x80000000.
 *
 * Layout (16 words = 64 bytes total):
 *   +0x00 – +0x1C : 8 × LDR PC instructions  (vector table)
 *   +0x20 – +0x3C : 8 × address words         (literal pool)
 *
 * VBAR (Vector Base Address Register) points here.
 *   Boot:    VBAR = PA 0x80000000 (identity / MMU off)
 *   Runtime: VBAR = VA 0xC0000000 (high map, same physical memory)
 * ============================================================ */

.section .vectors, "ax"
.global _start

_start:
    ldr     pc, _vec_reset           /* +0x00: Reset */
    ldr     pc, _vec_undef           /* +0x04: Undefined Instruction */
    ldr     pc, _vec_svc             /* +0x08: Supervisor Call */
    ldr     pc, _vec_pabort          /* +0x0C: Prefetch Abort */
    ldr     pc, _vec_dabort          /* +0x10: Data Abort */
    ldr     pc, _vec_reserved        /* +0x14: Reserved */
    ldr     pc, _vec_irq             /* +0x18: IRQ */
    ldr     pc, _vec_fiq             /* +0x1C: FIQ */

/* Literal pool — handler addresses filled by linker */
_vec_reset:     .word reset_handler
_vec_undef:     .word exception_entry_undefined
_vec_svc:       .word exception_entry_svc
_vec_pabort:    .word exception_entry_prefetch_abort
_vec_dabort:    .word exception_entry_data_abort
_vec_reserved:  .word 0
_vec_irq:       .word exception_entry_irq
_vec_fiq:       .word exception_entry_fiq