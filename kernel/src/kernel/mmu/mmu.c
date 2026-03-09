/* ============================================================
 * mmu.c
 * ------------------------------------------------------------
 * MMU initialization — identity mapping (VA == PA)
 * L1 Section descriptors, 1MB granularity, no L2 tables.
 * ============================================================ */

#include "mmu.h"
#include "uart.h"
#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * L1 Page Table
 * ============================================================ */

/* 4096 entries × 4 bytes = 16KB, must be 16KB aligned for TTBR0.
 * Placed in .pgd section; zero-init by BSS clear → default FAULT. */
static uint32_t pgd[MMU_L1_ENTRIES]
    __attribute__((aligned(MMU_L1_ALIGN), section(".pgd")));

/* ============================================================
 * External ASM
 * ============================================================ */
extern void mmu_enable(uint32_t *pgd_base);

/* ============================================================
 * Page Table Builder
 * ============================================================ */
static void mmu_build_page_table(void)
{
    uint32_t i;
    uint32_t section_base;

    /* Clear all → Translation Fault (catches NULL pointers) */
    for (i = 0; i < MMU_L1_ENTRIES; i++)
    {
        pgd[i] = 0;
    }

    /* L4_WKUP peripherals: UART0, CM_PER (1MB) */
    for (i = 0; i < PERIPH_L4_WKUP_SECTIONS; i++)
    {
        section_base = PERIPH_L4_WKUP_BASE + (i * MMU_SECTION_SIZE);
        pgd[section_base >> MMU_SECTION_SHIFT] =
            section_base | MMU_SECT_PERIPHERAL;
    }

    /* L4_PER peripherals: INTC, DMTimer, GPIO (3MB) */
    for (i = 0; i < PERIPH_L4_PER_SECTIONS; i++)
    {
        section_base = PERIPH_L4_PER_BASE + (i * MMU_SECTION_SIZE);
        pgd[section_base >> MMU_SECTION_SHIFT] =
            section_base | MMU_SECT_PERIPHERAL;
    }

    /* User bootstrap DDR: current user code/stacks linked near 0x80000000 */
    for (i = 0; i < USER_BOOTSTRAP_MB; i++)
    {
        section_base = DDR_BASE + (i * MMU_SECTION_SIZE);
        pgd[section_base >> MMU_SECTION_SHIFT] =
            section_base | MMU_SECT_USER_RAM;
    }

    /* Kernel DDR: .text, .data, .bss, stacks, page table (8MB)
     * Kernel-only — user access triggers Domain fault */
    for (i = USER_BOOTSTRAP_MB; i < KERNEL_SIZE_MB; i++)
    {
        section_base = DDR_BASE + (i * MMU_SECTION_SIZE);
        pgd[section_base >> MMU_SECTION_SHIFT] =
            section_base | MMU_SECT_KERNEL_RAM;
    }

    /* User DDR: remaining 120MB, accessible from user mode */
    for (i = 0; i < USER_SIZE_MB; i++)
    {
        section_base = DDR_BASE + ((KERNEL_SIZE_MB + i) * MMU_SECTION_SIZE);
        pgd[section_base >> MMU_SECTION_SHIFT] =
            section_base | MMU_SECT_USER_RAM;
    }
}

/* ============================================================
 * mmu_init — Public API
 * ============================================================ */
void mmu_init(void)
{
    uart_printf("[MMU] Building L1 page table at %p\n", (void *)pgd);
    uart_printf("[MMU] Mapping %d entries (%d KB table)\n",
                MMU_L1_ENTRIES, (MMU_L1_ENTRIES * 4) / 1024);

    mmu_build_page_table();

    uart_printf("[MMU] Peripheral L4_WKUP: 0x%x (%d MB) [Strongly Ordered]\n",
                PERIPH_L4_WKUP_BASE, PERIPH_L4_WKUP_SECTIONS);
    uart_printf("[MMU] Peripheral L4_PER:  0x%x (%d MB) [Strongly Ordered]\n",
                PERIPH_L4_PER_BASE, PERIPH_L4_PER_SECTIONS);
    uart_printf("[MMU] User Bootstrap DDR: 0x%x (%d MB) [Cached, User RW]\n",
                DDR_BASE, USER_BOOTSTRAP_MB);
    uart_printf("[MMU] Kernel DDR:  0x%x (%d MB) [Cached, Kernel-only]\n",
                DDR_BASE + (USER_BOOTSTRAP_MB * MMU_SECTION_SIZE),
                KERNEL_SIZE_MB - USER_BOOTSTRAP_MB);
    uart_printf("[MMU] User DDR:    0x%x (%d MB) [Cached, User RW]\n",
                DDR_BASE + (KERNEL_SIZE_MB * MMU_SECTION_SIZE), USER_SIZE_MB);
    uart_printf("[MMU] DACR = 0x%x (D0=CLIENT, D1=CLIENT)\n", MMU_DACR_VALUE);

    uart_printf("[MMU] Enabling MMU...\n");
    mmu_enable(pgd);
    uart_printf("[MMU] MMU enabled successfully!\n");
}
