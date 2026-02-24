/* ============================================================
 * bootloader.ld
 * Linker Script for AM335x Bootloader
 * Target: SRAM @ 0x402F0400
 * ============================================================ */

ENTRY(_start)

MEMORY
{
    SRAM (rwx) : ORIGIN = 0x402F0400, LENGTH = 0x0001B800
}

SECTIONS
{
    /* Vector table must be at beginning */
    .vectors :
    {
        _vectors_start = .;
        *(.vectors)
        . = ALIGN(32); /* Ensure 32-byte alignment for VBAR */
        _vectors_end = .;
    } > SRAM

    .text :
    {
        _text_start = .;
        *(.text.start)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _text_end = .;
    } > SRAM

    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data*)
        . = ALIGN(4);
        _data_end = .;
    } > SRAM

    .bss (NOLOAD) :
    {
        . = ALIGN(8);
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > SRAM

    /* Stack section for all processor modes */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _stack_start = .;
        
        /* FIQ stack (512 bytes) */
        . = . + 0x200;
        PROVIDE(_fiq_stack_top = .);
        
        /* IRQ stack (1KB) */
        . = . + 0x400;
        PROVIDE(_irq_stack_top = .);
        
        /* Abort stack (1KB) */
        . = . + 0x400;
        PROVIDE(_abt_stack_top = .);
        
        /* Undefined stack (1KB) */
        . = . + 0x400;
        PROVIDE(_und_stack_top = .);
        
        /* Supervisor (SVC) stack (8KB) - main bootloader stack */
        . = . + 0x2000;
        PROVIDE(_svc_stack_top = .);
        
        /* System stack (1KB) */
        . = . + 0x400;
        PROVIDE(_sys_stack_top = .);
        
        _stack_end = .;
    } > SRAM

    PROVIDE(_bootloader_end = .);
    PROVIDE(_heap_start = .);
}
