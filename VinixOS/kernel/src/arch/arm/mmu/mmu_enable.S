/* ============================================================
 * mmu_enable.S
 * ------------------------------------------------------------
 * MMU enable sequence
 *
 * Placed in .text.mmu_trampoline so VMA == LMA == PA.
 * Called from entry.S (also at PA) before the jump to high VA.
 * After this function returns, both identity map (PA→PA) and
 * high map (PA→VA 0xC0xxxxxx) are active.
 * ============================================================ */

    .section .text.mmu_trampoline, "ax"
    .align 4
    .global mmu_enable

/* ------------------------------------------------------------
 * mmu_enable(uint32_t *pgd_base)
 * ------------------------------------------------------------
 * r0 = physical address of L1 page table (16KB aligned)
 * Clobbers: r0, r1
 * ------------------------------------------------------------ */
mmu_enable:

    /* Drain pending writes before touching MMU config */
    dsb

    /* Invalidate I-Cache — stale entries from MMU-off state
     * would have wrong cacheability attributes after MMU on */
    mov     r1, #0
    mcr     p15, 0, r1, c7, c5, 0      /* ICIALLU */
    dsb
    isb

    /* TTBR0 = page table base + table walk cacheability
     * 0x4A = Inner WB/WA (bit[6]=1), Outer WB/WA (bit[3]=1),
     *        Shareable (bit[1]=1) */
    orr     r0, r0, #0x4A
    mcr     p15, 0, r0, c2, c0, 0      /* Write TTBR0 */

    /* TTBCR = 0 → TTBR0 covers entire 4GB, short-descriptor */
    mov     r1, #0
    mcr     p15, 0, r1, c2, c0, 2      /* Write TTBCR */

    /* DACR: D0=Client, D1=Client, rest=No Access
     * Client enforces AP bits; Manager would bypass all checks */
    mov     r1, #0x5
    mcr     p15, 0, r1, c3, c0, 0      /* Write DACR */

    /* Flush TLB — no stale translations before first use */
    mov     r1, #0
    mcr     p15, 0, r1, c8, c7, 0      /* TLBIALL */
    dsb
    isb

    /* Enable MMU + D-Cache + I-Cache + branch prediction
     * M+C must be set together — C without M is UNPREDICTABLE */
    mrc     p15, 0, r1, c1, c0, 0      /* Read SCTLR */
    orr     r1, r1, #(1 << 0)          /* M — MMU */
    orr     r1, r1, #(1 << 2)          /* C — D-Cache */
    orr     r1, r1, #(1 << 11)         /* Z — Branch prediction */
    orr     r1, r1, #(1 << 12)         /* I — I-Cache */
    mcr     p15, 0, r1, c1, c0, 0      /* Write SCTLR */

    /* Pipeline sync — next fetch goes through MMU.
     * Identity map (PA→PA) keeps this code reachable. */
    isb

    bx      lr
