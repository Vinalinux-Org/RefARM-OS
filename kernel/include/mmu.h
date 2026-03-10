/* ============================================================
 * mmu.h
 * ------------------------------------------------------------
 * MMU definitions and interface — 3G/1G Virtual Memory Split
 *
 * Virtual Address Layout:
 *   User  Space: 0x00000000 – 0xBFFFFFFF (3GB)
 *   Kernel Space: 0xC0000000 – 0xFFFFFFFF (1GB)
 *
 * Physical DDR: 0x80000000 (128MB)
 *   → Mapped to VA 0xC0000000 (Kernel)
 *
 * Peripherals: identity mapped (PA == VA, below 0xC0000000)
 *   but access restricted to Kernel only (AP=01).
 * ============================================================ */

#ifndef MMU_H
#define MMU_H

#include <stdint.h>

/* ============================================================
 * L1 Section Descriptor Bit Fields
 * ============================================================
 *
 * 31        20 19 18 17 16 15 14:12 11:10 9 8:5 4 3 2 1 0
 * [base addr ] NS  0  nG  S APX TEX  AP  imp Domain XN C B 1 0
 * ============================================================ */

#define MMU_DESC_SECTION (0x2) /* bits[1:0] = 10 → Section */

/* ============================================================
 * Access Permission (APX=0)
 * ============================================================ */
#define MMU_AP_KERN_RW (0x1 << 10)     /* AP=01: Kernel RW, User no-access */
#define MMU_AP_FULL_ACCESS (0x3 << 10) /* AP=11: Kernel RW, User RW */

/* ============================================================
 * Domain — bits[8:5], 16 domains (0–15)
 * ============================================================ */
#define MMU_DOMAIN(d) (((d) & 0xF) << 5)
#define MMU_DOMAIN_KERNEL 0
#define MMU_DOMAIN_USER 1

/* ============================================================
 * TEX, C, B — Memory Attributes (no TEX remap)
 * ============================================================ */
#define MMU_TEX(t) (((t) & 0x7) << 12)
#define MMU_CACHED (1 << 3)     /* C bit */
#define MMU_BUFFERED (1 << 2)   /* B bit */
#define MMU_SHAREABLE (1 << 16) /* S bit */
#define MMU_XN (1 << 4)         /* Execute-Never */

/* ============================================================
 * Composite Section Attributes
 * ============================================================ */

/* Strongly Ordered, Kernel-only, no execute (peripherals) */
#define MMU_SECT_PERIPHERAL \
    (MMU_DESC_SECTION | MMU_AP_KERN_RW | MMU_DOMAIN(MMU_DOMAIN_KERNEL) | MMU_TEX(0) | MMU_XN)

/* Normal Cached WB/WA, Kernel-only (kernel code + data) */
#define MMU_SECT_KERNEL_RAM \
    (MMU_DESC_SECTION | MMU_AP_KERN_RW | MMU_DOMAIN(MMU_DOMAIN_KERNEL) | MMU_TEX(1) | MMU_CACHED | MMU_BUFFERED | MMU_SHAREABLE)

/* Normal Cached WB/WA, Kernel+User RW (user memory) */
#define MMU_SECT_USER_RAM \
    (MMU_DESC_SECTION | MMU_AP_FULL_ACCESS | MMU_DOMAIN(MMU_DOMAIN_USER) | MMU_TEX(1) | MMU_CACHED | MMU_BUFFERED | MMU_SHAREABLE)

/* ============================================================
 * DACR — Domain Access Control
 * ============================================================ */
#define DACR_NO_ACCESS 0x0
#define DACR_CLIENT 0x1  /* Enforce AP bits */
#define DACR_MANAGER 0x3 /* Bypass AP checks */

/* D0=Client, D1=Client, D2–15=No access → 0x00000005 */
#define MMU_DACR_VALUE \
    ((DACR_CLIENT << (MMU_DOMAIN_KERNEL * 2)) | (DACR_CLIENT << (MMU_DOMAIN_USER * 2)))

/* ============================================================
 * SCTLR Bits
 * ============================================================ */
#define SCTLR_M (1 << 0)  /* MMU enable */
#define SCTLR_A (1 << 1)  /* Alignment check */
#define SCTLR_C (1 << 2)  /* D-Cache enable */
#define SCTLR_Z (1 << 11) /* Branch prediction */
#define SCTLR_I (1 << 12) /* I-Cache enable */

/* ============================================================
 * Page Table Constants
 * ============================================================ */
#define MMU_L1_ENTRIES 4096
#define MMU_L1_ALIGN 16384        /* 16KB alignment */
#define MMU_SECTION_SIZE 0x100000 /* 1MB */
#define MMU_SECTION_SHIFT 20

/* ============================================================
 * 3G/1G Virtual Memory Layout
 * ============================================================ */

/* Kernel virtual address base (1GB kernel window) */
#define KERNEL_VA_BASE 0xC0000000

/* True User Space virtual address base */
#define USER_VA_BASE 0x40000000

/* Physical DDR base (AM335x) */
#define DDR_PA_BASE 0x80000000
#define DDR_SIZE_MB 128

/* PA ↔ VA conversion offset */
#define VA_OFFSET (KERNEL_VA_BASE - DDR_PA_BASE) /* 0x40000000 */

/* Convert between physical and virtual addresses (kernel space) */
#define PA_TO_VA(pa) ((pa) + VA_OFFSET)
#define VA_TO_PA(va) ((va) - VA_OFFSET)

/* Kernel DDR: The first 5MB is for Kernel */
#define KERNEL_DDR_VA KERNEL_VA_BASE
#define KERNEL_DDR_PA DDR_PA_BASE
#define KERNEL_DDR_MB 5

/* User App DDR: Starts after Kernel (Memory pool)
 * PA 0x80500000 -> VA 0x40000000 (1MB for User App) */
#define USER_SPACE_PA (DDR_PA_BASE + (KERNEL_DDR_MB * MMU_SECTION_SIZE))
#define USER_SPACE_VA USER_VA_BASE
#define USER_SPACE_MB 1

/* ============================================================
 * Peripheral Physical Addresses (identity mapped)
 * ============================================================
 * Peripherals are mapped PA == VA (below 0xC0000000) with
 * AP=Kernel-only. This avoids remapping every driver base
 * address while still protecting User mode from accessing I/O.
 */

/* L4_WKUP: UART0 (0x44E09000), CM_PER (0x44E00000), WDT1 (0x44E35000) */
#define PERIPH_L4_WKUP_PA 0x44E00000
#define PERIPH_L4_WKUP_SECTIONS 1

/* L4_PER: INTC (0x48200000), DMTIMER2 (0x48040000) */
#define PERIPH_L4_PER_PA 0x48000000
#define PERIPH_L4_PER_SECTIONS 3

/* ============================================================
 * Boot-time Temporary Identity Mapping
 * ============================================================
 * During the MMU trampoline, we need both identity (PA==VA)
 * and high (PA→VA 0xC0xxxxxx) mappings for the DDR range.
 * After jumping to high VA, the identity map is removed.
 */
#define BOOT_IDENTITY_PA DDR_PA_BASE
#define BOOT_IDENTITY_MB DDR_SIZE_MB /* 128MB temporary */

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Build L1 page table and enable MMU + caches.
 * Includes the trampoline: identity map → enable MMU →
 * jump to high VA → remove identity map.
 *
 * Must be called after uart_init(), before intc_init().
 * After this function returns, the kernel runs at VA 0xC0xxxxxx.
 */
void mmu_init(void);

#endif /* MMU_H */
