/* ============================================================
 * vectors.S
 * ------------------------------------------------------------
 * ARM Exception Vector Table
 * Architecture: ARMv7-A (Cortex-A8)
 * Base Address: Configured via VBAR (e.g., 0x80000000)
 * ============================================================ */

/* ============================================================
 * Vector Table Layout (VBAR-based)
 * ------------------------------------------------------------
 * Each entry is 4 bytes (one ARM instruction)
 * Total size: 32 bytes (0x20)
 * 
 * Offsets are relative to VBAR:
 *   +0x00: Reset
 *   +0x04: Undefined Instruction
 *   +0x08: Supervisor Call (SVC)
 *   +0x0C: Prefetch Abort
 *   +0x10: Data Abort
 *   +0x14: Reserved (not used)
 *   +0x18: IRQ
 *   +0x1C: FIQ
 * 
 * CRITICAL: This table MUST be placed at the start of the
 * kernel image (section .vectors) so linker puts it at
 * the address configured in VBAR during boot.
 * ============================================================ */

.section .vectors, "ax"
.global _start

_start:
    b       reset_handler                    /* +0x00: Reset */
    b       exception_entry_undefined        /* +0x04: Undefined Instruction */
    b       exception_entry_svc              /* +0x08: Supervisor Call */
    b       exception_entry_prefetch_abort   /* +0x0C: Prefetch Abort */
    b       exception_entry_data_abort       /* +0x10: Data Abort */
    nop                                      /* +0x14: Reserved */
    b       exception_entry_irq              /* +0x18: IRQ */
    b       exception_entry_fiq              /* +0x1C: FIQ */

/* ============================================================
 * Notes:
 * ------------------------------------------------------------
 * 1. reset_handler is defined in boot/reset.S
 * 2. exception_entry_* are defined in exceptions/exception_entry.S
 * 3. VBAR is configured in boot/reset.S to point to _start
 * 4. This is a pure jump table - no logic here
 * 5. Each branch can reach ±32MB (sufficient for kernel image)
 * ============================================================ */