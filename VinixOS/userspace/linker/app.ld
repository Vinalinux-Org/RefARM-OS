/* ============================================================
 * app.ld
 * ------------------------------------------------------------
 * Linker script for User Applications (VinixOS SDK)
 * Target: ARMv7-A
 * 
 * VMA Concept: User applications are built to run at 
 * Virtual Address 0x40000000 (lower 3GB).
 * 
 * The kernel will map a physical memory region to this VA,
 * copy the raw binary (`app.bin`) here, and set PC=0x40000000.
 * ============================================================ */

ENTRY(_start)

MEMORY
{
    /* User App Memory: starts at 0x40000000. 
     * Kernel allocates 1MB (by default) for the app. */
    RAM (rwx) : ORIGIN = 0x40000000, LENGTH = 1M
}

SECTIONS
{
    /* ========================================================
     * .text - Code & Read-only Data
     * ======================================================== */
    .text :
    {
        _text_start = .;
        
        /* Must put the startup code first so that _start is at 0x40000000 */
        *(.text.crt0)
        
        *(.text*)
        *(.rodata*)
        
        . = ALIGN(4);
        _text_end = .;
    } > RAM

    /* ========================================================
     * .data - Initialized Data
     * ======================================================== */
    .data :
    {
        _data_start = .;
        *(.data*)
        . = ALIGN(4);
        _data_end = .;
    } > RAM

    /* ========================================================
     * .bss - Uninitialized Data (Zeroed by crt0)
     * ======================================================== */
    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM
    
    /* End of image */
    _app_end = .;
}
