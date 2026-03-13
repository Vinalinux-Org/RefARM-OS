/* ============================================================
 * entry.S
 * ------------------------------------------------------------
 * Boot entry — BSS clear, MMU enable, trampoline to high VA
 *
 * Runs at PA 0x80000000+ (in .text.boot_entry, VMA == LMA).
 * After enabling the MMU and reloading stack pointers, this
 * code jumps to kernel_main() at VA 0xC0xxxxxx.
 *
 * Boot flow:
 *   1. Clear .bss (using PA-converted addresses)
 *   2. Build page table (call C helper at PA)
 *   3. Enable MMU (identity + high maps active)
 *   4. Reload all stack pointers to VA (through identity map)
 *   5. Jump to kernel_main at VA ← THE TRAMPOLINE
 *
 * Assumptions:
 *   - CPU is in SVC mode, IRQ/FIQ masked
 *   - All exception stacks set to PA in reset.S
 *   - MMU and cache are OFF
 * ============================================================ */

    .section .text.boot_entry, "ax"
    .align  4
    .global entry

.equ VA_OFFSET_BOOT, 0x40000000

entry:
    /* ========================================================
     * 1. Clear .bss section
     * ========================================================
     * _bss_start / _bss_end are at kernel VA (0xC0xxxxxx).
     * MMU is OFF → subtract VA_OFFSET to get PA.
     * ======================================================== */
    ldr     r0, =_bss_start
    sub     r0, r0, #VA_OFFSET_BOOT
    ldr     r1, =_bss_end
    sub     r1, r1, #VA_OFFSET_BOOT
    mov     r2, #0

1:  cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b

    /* ========================================================
     * 2. Build L1 page table (Phase A)
     * ========================================================
     * mmu_build_page_table_boot(uint32_t *pgd_pa) is a C
     * function placed in .text.boot_entry (VMA == PA).
     * Pass the physical address of the pgd array.
     * ======================================================== */
    ldr     r0, =_pgd_start
    sub     r0, r0, #VA_OFFSET_BOOT     /* pgd PA */
    bl      mmu_build_page_table_boot

    /* ========================================================
     * 3. Enable MMU
     * ========================================================
     * mmu_enable(uint32_t *pgd_base) is in .text.mmu_trampoline
     * (VMA == PA, within .text.boot output section).
     * After return: MMU ON, both identity map (PA→PA) and
     * high map (PA→VA 0xC0xxxxxx) are active.
     * We are still executing at PA (via identity map).
     * ======================================================== */
    ldr     r0, =_pgd_start
    sub     r0, r0, #VA_OFFSET_BOOT     /* pgd PA */
    bl      mmu_enable

    /* ========================================================
     * 4. Reload ALL stack pointers to VA
     * ========================================================
     * MMU is ON. Identity map keeps PA accessible.
     * Load VA values from linker symbols so stacks work
     * correctly after the identity map is removed.
     * ======================================================== */
    cps     #0x11                       /* FIQ mode */
    ldr     sp, =_fiq_stack_top

    cps     #0x12                       /* IRQ mode */
    ldr     sp, =_irq_stack_top

    cps     #0x17                       /* ABT mode */
    ldr     sp, =_abt_stack_top

    cps     #0x1B                       /* UND mode */
    ldr     sp, =_und_stack_top

    cps     #0x13                       /* SVC mode */
    ldr     sp, =_svc_stack_top

    /* ========================================================
     * 5. TRAMPOLINE — Jump to kernel_main at high VA
     * ========================================================
     * `ldr pc, =kernel_main` loads the VA (0xC0xxxxxx) from
     * a literal pool in .text.boot_entry (accessible via
     * identity map) and sets PC to that VA. The CPU then
     * fetches from VA through the high mapping.
     *
     * After this instruction, we are running in kernel VA
     * space. The identity map (PA→PA) is still active and
     * will be removed by mmu_init() in kernel_main().
     * ======================================================== */
    ldr     pc, =kernel_main

    /* Should never reach here */
halt:
    b       halt

    .extern _bss_start
    .extern _bss_end
    .extern _pgd_start
    .extern mmu_build_page_table_boot
    .extern mmu_enable
    .extern _fiq_stack_top
    .extern _irq_stack_top
    .extern _abt_stack_top
    .extern _und_stack_top
    .extern _svc_stack_top
    .extern kernel_main