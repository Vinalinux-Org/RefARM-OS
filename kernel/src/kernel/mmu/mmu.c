/* ============================================================
 * mmu.c
 * ------------------------------------------------------------
 * MMU initialization — 3G/1G Virtual Memory Split
 *
 * Two-phase design:
 *   Phase A (boot, at PA): entry.S calls mmu_build_page_table_boot()
 *     and mmu_enable_and_jump(). These run at PA before MMU enable.
 *     After MMU enable + trampoline, execution continues at high VA.
 *
 *   Phase B (runtime, at VA): kernel_main() calls mmu_init() to
 *     remove the temporary identity mapping and update VBAR.
 *
 * Uses L1 section descriptors with 1MB granularity, no L2 tables.
 * ============================================================ */

#include "mmu.h"
#include "uart.h"
#include "types.h"

/* ============================================================
 * L1 Page Table
 * ============================================================ */

/* 4096 entries × 4 bytes = 16KB, must be 16KB aligned.
 * Placed in .pgd section (kernel VA space).
 * Boot code accesses this at PA via mmu_build_page_table_boot().
 *
 * 'used' attribute prevents compiler from eliminating this array:
 * mmu_build_page_table_boot() populates it through a PA pointer,
 * and mmu_init() accesses it via this static symbol at VA.
 * Without 'used', compiler considers pgd dead code. */
static uint32_t pgd[MMU_L1_ENTRIES]
    __attribute__((aligned(MMU_L1_ALIGN), section(".pgd"), used));

/* ============================================================
 * Linker symbols
 * ============================================================ */
extern uint32_t _boot_start; /* PA 0x80000000 — vector table */

/* ============================================================
 * Phase B: mmu_init — called from kernel_main at high VA
 * ============================================================
 * By the time this runs, MMU is already ON and we are executing
 * at VA 0xC0xxxxxx. This function:
 *   1. Removes temporary identity mapping (PA == VA for DDR)
 *   2. Updates VBAR to point to vectors at high VA
 *   3. Logs the final memory map
 */
void mmu_init(void)
{
    uint32_t i, pa;

    /* Remove temporary identity mapping */
    for (i = 0; i < BOOT_IDENTITY_MB; i++)
    {
        pa = BOOT_IDENTITY_PA + (i * MMU_SECTION_SIZE);
        pgd[pa >> MMU_SECTION_SHIFT] = 0; /* FAULT */
    }

    /* Flush entire TLB - clear stale identity-map entries */
    __asm__ __volatile__(
        "mov r0, #0\n\t"
        "mcr p15, 0, r0, c8, c7, 0\n\t" /* TLBIALL */
        "dsb\n\t"
        "isb\n\t" ::: "r0", "memory");

    /* Update VBAR to high VA
     * Vector table physical location = _boot_start (PA 0x80000000).
     * With the high mapping, same memory is at VA 0xC0000000.
     * VBAR must point to VA so exception vectors resolve correctly. */
    uint32_t vbar_va = (uint32_t)&_boot_start + VA_OFFSET;

    __asm__ __volatile__(
        "mcr p15, 0, %0, c12, c0, 0\n\t" /* Write VBAR */
        "isb\n\t" ::"r"(vbar_va) : "memory");

    /* Log final memory map */
    uart_printf("[MMU] True 3G/1G Virtual Memory Split\n");
    uart_printf("[MMU] User Space:  VA 0x%x -> PA 0x%x (%d MB) [Cached, User RW]\n",
                USER_SPACE_VA, USER_SPACE_PA, USER_SPACE_MB);
    uart_printf("[MMU] Kernel DDR:  VA 0x%x -> PA 0x%x (%d MB) [Cached, Kernel-only]\n",
                KERNEL_DDR_VA, KERNEL_DDR_PA, KERNEL_DDR_MB);
    uart_printf("[MMU] Peripheral L4_WKUP: PA 0x%x (%d MB) [Strongly Ordered, Identity]\n",
                PERIPH_L4_WKUP_PA, PERIPH_L4_WKUP_SECTIONS);
    uart_printf("[MMU] Peripheral L4_PER:  PA 0x%x (%d MB) [Strongly Ordered, Identity]\n",
                PERIPH_L4_PER_PA, PERIPH_L4_PER_SECTIONS);
    uart_printf("[MMU] Identity mapping removed (VA 0x80000000 now unmapped)\n");
    uart_printf("[MMU] VBAR = 0x%x\n", vbar_va);
    uart_printf("[MMU] DACR = 0x%x (D0=CLIENT, D1=CLIENT)\n", MMU_DACR_VALUE);
    uart_printf("[MMU] MMU enabled, running at high VA!\n");
}

/* ============================================================
 * Phase A: Boot-time Page Table Builder
 * ============================================================
 * Called from entry.S at PA, BEFORE MMU enable.
 * Receives pgd physical address as argument (not the VA symbol).
 * Placed in .text.boot_entry so VMA == LMA == PA.
 *
 * Memory mapping:
 *   PA 0x80000000 → VA 0xC0000000 (1MB, User bootstrap) [permanent]
 *   PA 0x80100000 → VA 0xC0100000 (127MB, Kernel-only)  [permanent]
 *   PA 0x80000000 → VA 0x80000000 (128MB, identity)     [temporary]
 *   PA 0x44E00000 → VA 0x44E00000 (1MB, peripheral)     [permanent]
 *   PA 0x48000000 → VA 0x48000000 (3MB, peripheral)     [permanent]
 *
 * TRUE 3G/1G SPLIT:
 *   Kernel no longer shares 0xC0000000 with User tasks.
 *   User tasks receive their own dedicated mapping at 0x40000000.
 */
void __attribute__((section(".text.boot_entry")))
mmu_build_page_table_boot(uint32_t *pgd_pa)
{
    uint32_t i;
    uint32_t pa, va_idx;

    /* Clear all entries → Translation Fault (catches NULL pointers) */
    for (i = 0; i < MMU_L1_ENTRIES; i++)
    {
        pgd_pa[i] = 0;
    }

    /* Peripheral identity maps (PA == VA, Kernel-only) */

    /* L4_WKUP: UART0, CM_PER, WDT1 (1MB) */
    for (i = 0; i < PERIPH_L4_WKUP_SECTIONS; i++)
    {
        pa = PERIPH_L4_WKUP_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }

    /* L4_PER: INTC, DMTimer, GPIO (3MB) */
    for (i = 0; i < PERIPH_L4_PER_SECTIONS; i++)
    {
        pa = PERIPH_L4_PER_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }

    /* True User Space: VA 0x40000000 (User RW) */
    for (i = 0; i < USER_SPACE_MB; i++)
    {
        pa = USER_SPACE_PA + (i * MMU_SECTION_SIZE);
        va_idx = (USER_SPACE_VA + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_USER_RAM;
    }

    /* Kernel DDR: VA 0xC0000000 (Kernel-only) */
    for (i = 0; i < KERNEL_DDR_MB; i++)
    {
        pa = KERNEL_DDR_PA + (i * MMU_SECTION_SIZE);
        va_idx = (KERNEL_DDR_VA + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_KERNEL_RAM;
    }

    /* Temporary identity map: PA 0x80000000 → VA 0x80000000
     * Allows CPU to continue executing at PA after MMU enable.
     * Removed later by mmu_init() once we are at high VA. */
    for (i = 0; i < BOOT_IDENTITY_MB; i++)
    {
        pa = BOOT_IDENTITY_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_KERNEL_RAM;
    }
}
