/* ============================================================
 * Linker Script — 3G/1G Virtual Memory Split
 * Target : AM335x / Cortex-A8
 *
 * Memory Model:
 *  - Bootloader loads kernel binary at PA 0x80000000
 *  - Boot code (.text.boot) runs at PA == VA (identity mapped)
 *  - Kernel code/data runs at VA 0xC0000000 (mapped to PA 0x80000000)
 *  - User space: VA 0x00000000–0xBFFFFFFF (3GB)
 *  - Kernel space: VA 0xC0000000–0xFFFFFFFF (1GB)
 *
 * Critical: .text.boot section has VMA == LMA == 0x80000000
 *           All other kernel sections have VMA starting at 0xC0xxxxxx
 *           but LMA (load address) in DDR after .text.boot.
 * ============================================================ */

/* Entry point */
ENTRY(_start)

/* ============================================================
 * Physical memory region (where bootloader loads the binary)
 * ============================================================ */
MEMORY
{
    DDR (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* ============================================================
 * Virtual address constants
 * ============================================================ */
KERNEL_VA_BASE = 0xC0000000;
KERNEL_PA_BASE = 0x80000000;
VA_OFFSET      = KERNEL_VA_BASE - KERNEL_PA_BASE;   /* 0x40000000 */

/* ============================================================
 * Section layout
 * ============================================================ */
SECTIONS
{
    /* ========================================================
     * BOOT CODE — VMA == LMA == PA 0x80000000
     * ========================================================
     * Runs with MMU OFF (or identity-mapped during trampoline).
     * Contains vector table, reset handler, entry stub, and
     * the MMU trampoline that jumps to kernel high VA.
     *
     * IMPORTANT: Code here must NOT reference kernel symbols
     * that live at VA 0xC0xxxxxx (those addresses are invalid
     * until MMU maps them). Use only local labels or PA symbols.
     * ======================================================== */
    .text.boot ORIGIN(DDR) :
    {
        _boot_start = .;

        /* Vector table — first 32 bytes, pointed to by VBAR */
        KEEP(*(.vectors))

        /* Reset handler (stack setup, VBAR config) */
        KEEP(*(.text.reset))

        /* Entry stub (BSS clear, call kernel_main) */
        KEEP(*(.text.boot_entry))

        /* MMU enable + trampoline code */
        KEEP(*(.text.mmu_trampoline))

        . = ALIGN(4);
        _boot_end = .;
    } > DDR

    /* PA marker — physical address right after boot code.
     * Used to calculate LMA for the first kernel VA section. */
    _kernel_lma_start = _boot_end;

    /* ========================================================
     * KERNEL CODE — VMA = 0xC0xxxxxx, LMA = after .text.boot
     * ========================================================
     * The linker resolves all symbols to VA (0xC0xxxxxx).
     * The binary is physically placed after .text.boot in DDR.
     * The MMU trampoline enables the mapping VA→PA before
     * any of this code executes.
     *
     * IMPORTANT: Every section below uses an explicit VMA
     * expression (ADDR + SIZEOF of the previous section) so
     * that the linker never relies on the '.' location counter
     * after the VMA/LMA split introduced by AT(). Without
     * this, GNU ld can reset '.' to track LMA instead of VMA,
     * placing .bss/.pgd/.stack at PA 0x80xxxxxx instead of
     * the intended kernel VA 0xC0xxxxxx.
     * ======================================================== */
    .text (KERNEL_VA_BASE + (_kernel_lma_start - KERNEL_PA_BASE)) : AT(_kernel_lma_start)
    {
        _text_start = .;

        /* Kernel code */
        *(.text*)
        *(.rodata*)

        _text_end = .;
    }

    /* ========================================================
     * Initialized data
     * ======================================================== */
    .data ALIGN(ADDR(.text) + SIZEOF(.text), 4) : AT(ALIGN(LOADADDR(.text) + SIZEOF(.text), 4))
    {
        _data_start = .;
        *(.data*)
        _data_end = .;
    }

    /* ========================================================
     * BSS (zero-initialized)
     * ======================================================== */
    .bss ALIGN(ADDR(.data) + SIZEOF(.data), 4) (NOLOAD) :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    }

    /* ========================================================
     * L1 Page Table (16KB, 16KB-aligned)
     * ARM ARM B4.1.154 — TTBR0 requires 14-bit alignment.
     * Zero-init by BSS clear → all entries default to FAULT.
     * ======================================================== */
    .pgd ALIGN(ADDR(.bss) + SIZEOF(.bss), 16384) (NOLOAD) :
    {
        _pgd_start = .;
        *(.pgd)
        . = ALIGN(16384);
        _pgd_end = .;
    }

    /* ========================================================
     * User Stacks (Isolated Region)
     * Pointer validation: _user_stack_start <= ptr < _user_stack_end
     * ======================================================== */
    .user_stack ALIGN(ADDR(.pgd) + SIZEOF(.pgd), 4096) (NOLOAD) :
    {
        _user_stack_start = .;
        *(.user_stack)
        . = ALIGN(4096);
        _user_stack_end = .;
    }

    /* ========================================================
     * Exception Mode Stacks
     * Stack grows downward (high → low).
     *   FIQ   512 bytes  (not used, minimal)
     *   IRQ   4 KB
     *   ABT   4 KB       (Data/Prefetch Abort)
     *   UND   4 KB       (Undefined Instruction)
     *   SVC   8 KB       (main kernel stack)
     *
     * CRITICAL: Must be ready before VBAR is set in reset.S.
     * ======================================================== */
    .stack ALIGN(ADDR(.user_stack) + SIZEOF(.user_stack), 8) (NOLOAD) :
    {
        _stack_start = .;

        . = . + 0x200;
        _fiq_stack_top = .;

        . = . + 0x1000;
        _irq_stack_top = .;

        . = . + 0x1000;
        _abt_stack_top = .;

        . = . + 0x1000;
        _und_stack_top = .;

        . = . + 0x2000;
        _svc_stack_top = .;

        _stack_end = .;
    }

    /* ========================================================
     * End of kernel image
     * ======================================================== */
    _kernel_end = ADDR(.stack) + SIZEOF(.stack);

    /* ========================================================
     * Helper symbols for PA/VA conversion
     * Used by boot code and MMU trampoline.
     * ======================================================== */
    _kernel_va_base  = KERNEL_VA_BASE;
    _kernel_pa_base  = KERNEL_PA_BASE;
    _va_offset       = VA_OFFSET;
}