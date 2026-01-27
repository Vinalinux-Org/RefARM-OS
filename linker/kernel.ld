/* ============================================================
 * Linker Script for OS Reference (Boot Stage)
 * Target : AM335x / Cortex-A8
 *
 * Scope:
 *  - Boot until kernel_main()
 *  - MMU OFF, physical == virtual
 * ============================================================ */

/* Entry point */
ENTRY(_start)

/* ------------------------------------------------------------
 * Memory regions
 * ------------------------------------------------------------
 * External DDR
 * Base   : 0x80000000
 * Size   : 128 MB (lower window only)
 */
MEMORY
{
    DDR (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* ------------------------------------------------------------
 * Section layout
 * ------------------------------------------------------------
 */
SECTIONS
{
    /* --------------------------------------------------------
     * Vector table
     * --------------------------------------------------------
     * Vector table is placed in DDR and relocated via VBAR.
     */
    .vectors ORIGIN(DDR) :
    {
        _vector_start = .;
        KEEP(*(.vectors))
        _vector_end = .;
    } > DDR

    /* --------------------------------------------------------
     * Text section (code + rodata)
     * --------------------------------------------------------
     * Boot-critical code is placed first to ensure
     * deterministic execution order.
     */
    .text :
    {
        _text_start = .;

        /* Boot code */
        KEEP(*(.text.reset))
        KEEP(*(.text.boot_entry))

        /* Kernel code */
        *(.text*)
        *(.rodata*)

        _text_end = .;
    } > DDR

    /* --------------------------------------------------------
     * Initialized data
     * --------------------------------------------------------
     */
    .data :
    {
        _data_start = .;
        *(.data*)
        _data_end = .;
    } > DDR

    /* --------------------------------------------------------
     * BSS (zero-initialized data)
     * --------------------------------------------------------
     */
    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > DDR

    /* --------------------------------------------------------
     * Stack section
     * --------------------------------------------------------
     * Stack grows downward.
     * Layout (low -> high):
     *   [ FIQ stack ]   - 512 bytes (not used, minimal)
     *   [ IRQ stack ]   - 4 KB
     *   [ ABT stack ]   - 4 KB (for Data/Prefetch Abort)
     *   [ UND stack ]   - 4 KB (for Undefined Instruction)
     *   [ SVC stack ]   - 8 KB (main kernel stack)
     * 
     * CRITICAL: Exception modes MUST have valid stacks
     * before any exceptions can be handled!
     */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);

        _stack_start = .;

        /* FIQ stack (512 bytes - minimal, not used) */
        . = . + 0x200;
        _fiq_stack_top = .;

        /* IRQ stack (4 KB) */
        . = . + 0x1000;
        _irq_stack_top = .;

        /* ABT stack (4 KB - for Data/Prefetch Abort) */
        . = . + 0x1000;
        _abt_stack_top = .;

        /* UND stack (4 KB - for Undefined Instruction) */
        . = . + 0x1000;
        _und_stack_top = .;

        /* SVC stack (8 KB - main kernel stack) */
        . = . + 0x2000;
        _svc_stack_top = .;

        _stack_end = .;
    } > DDR

    /* --------------------------------------------------------
     * End of kernel image
     * --------------------------------------------------------
     */
    _kernel_end = .;
}