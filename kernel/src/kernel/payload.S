/* ============================================================
 * payload.S
 * ------------------------------------------------------------
 * Embeds the standalone User Application (SDK payload)
 * and filesystem files directly into the kernel's read-only data section.
 * ============================================================ */

.syntax unified
.section .rodata

/* ============================================================
 * User Application Binary
 * ============================================================ */
.global _shell_payload_start
.global _shell_payload_end

.align 4
_shell_payload_start:
    .incbin "../userspace/build/apps/shell/shell.bin"
_shell_payload_end:

/* ============================================================
 * RAMFS Embedded Files
 * ============================================================ */

/* hello.txt */
.global _ramfs_hello_start
.global _ramfs_hello_end
.align 4
_ramfs_hello_start:
    .incbin "src/kernel/files/hello.txt"
_ramfs_hello_end:

/* info.txt */
.global _ramfs_info_start
.global _ramfs_info_end
.align 4
_ramfs_info_start:
    .incbin "src/kernel/files/info.txt"
_ramfs_info_end:
