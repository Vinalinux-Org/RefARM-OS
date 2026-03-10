/* ============================================================
 * payload.S
 * ------------------------------------------------------------
 * Embeds the standalone User Application (SDK payload)
 * directly into the kernel's read-only data section.
 * ============================================================ */

.syntax unified
.section .rodata

.global _shell_payload_start
.global _shell_payload_end

.align 4
_shell_payload_start:
    .incbin "../userspace/build/apps/shell/shell.bin"
_shell_payload_end:
