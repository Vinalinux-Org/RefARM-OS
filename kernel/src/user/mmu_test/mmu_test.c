/* ============================================================
 * mmu_test.c
 * ------------------------------------------------------------
 * User Mode MMU Protection Validation Task
 *
 * PURPOSE:
 *   Verify that Kernel/User memory isolation enforced by the
 *   MMU actually works. This task runs in User Mode (USR) and
 *   deliberately attempts to read a Kernel-only memory region.
 *
 * EXPECTED BEHAVIOR:
 *   The read triggers a Data Abort (Permission Fault, Section).
 *   The exception handler terminates this task and resumes
 *   the scheduler. Shell and Idle tasks remain unaffected.
 *
 * MMU MAPPING CONTEXT (3G/1G Virtual Memory Split):
 *   VA 0xC0000000 (1MB):   USER_RAM   — AP=11, Domain 1 (User RW)  [bootstrap]
 *   VA 0xC0100000 (127MB): KERNEL_RAM — AP=01, Domain 0 (Kernel Only)
 *   VA 0x80000000:         UNMAPPED   — identity map removed by mmu_init()
 *
 * TARGET ADDRESS: 0xC0100000 (first byte of Kernel-only region at high VA)
 *   Expected fault: DFSR FS=0xD (Permission fault, Section)
 *   Expected DFAR:  0xC0100000
 *   Expected WnR:   0 (READ)
 * ============================================================ */

#include "user_syscall.h"
#include <stdint.h>

/* ============================================================
 * Output Helper
 * ------------------------------------------------------------
 * sys_write() validates that the buffer pointer resides in
 * User Stack memory (.user_stack section). String literals
 * live in .rodata (inside .text), so we must copy them to a
 * stack-local buffer before writing through the syscall.
 * ============================================================ */
static void test_puts(const char *s)
{
    char buf[64];
    int i = 0;

    while (*s)
    {
        if (*s == '\n')
        {
            /* Flush before newline */
            if (i > 0)
            {
                sys_write(buf, i);
                i = 0;
            }
            /* Emit \r\n */
            buf[0] = '\r';
            buf[1] = '\n';
            sys_write(buf, 2);
            s++;
            continue;
        }

        buf[i++] = *s++;

        if (i >= 64)
        {
            sys_write(buf, i);
            i = 0;
        }
    }

    if (i > 0)
    {
        sys_write(buf, i);
    }
}

/* ============================================================
 * Hex printer for a 32-bit value (stack-safe)
 * ============================================================ */
static void test_put_hex(uint32_t val)
{
    char buf[11]; /* "0x" + 8 hex digits + NUL */
    const char hex[] = "0123456789ABCDEF";

    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--)
    {
        buf[2 + (7 - i)] = hex[(val >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';

    sys_write(buf, 10);
}

/* ============================================================
 * MMU Test Task Entry Point
 * ============================================================ */
void mmu_test_task_entry(void)
{
    /* 1. Announce test start */
    test_puts("\n");
    test_puts("========================================\n");
    test_puts(" MMU Protection Validation Test\n");
    test_puts("========================================\n");

    /* 2. Report current CPU mode */
    uint32_t cpsr;
    __asm__ __volatile__("mrs %0, cpsr" : "=r"(cpsr));
    test_puts("[MMU_TEST] CPSR = ");
    test_put_hex(cpsr);
    test_puts(", Mode = ");
    test_put_hex(cpsr & 0x1F);
    test_puts(" (expect 0x10 = USR)\n");

    /* 3. Announce the dangerous read */
    test_puts("[MMU_TEST] Target: Kernel region at ");
    test_put_hex(0xC0100000);
    test_puts("\n");
    test_puts("[MMU_TEST] This address is mapped as KERNEL_RAM (AP=01)\n");
    test_puts("[MMU_TEST] A User-mode READ should trigger Data Abort.\n");
    test_puts("\n");
    test_puts("[MMU_TEST] >>> Attempting forbidden read NOW <<<\n");

    /* 4. The forbidden read — triggers Permission Fault (Section)
     *    CPU in USR mode, target section has AP=01 (kernel-only).
     *    DACR Domain 0 = CLIENT → AP bits enforced.
     *    Expected: Data Abort, DFSR FS=0xD, DFAR=0xC0100000 */
    volatile uint32_t *kernel_addr = (volatile uint32_t *)0xC0100000;
    volatile uint32_t val = *kernel_addr;

    /* ============================================================
     * SHOULD NEVER REACH HERE
     * If we get past the read, MMU protection has FAILED.
     * ============================================================ */
    (void)val;
    test_puts("\n");
    test_puts("!!! CRITICAL: Read succeeded — MMU protection BROKEN !!!\n");
    test_puts("[MMU_TEST] Value read: ");
    test_put_hex(val);
    test_puts("\n");
    sys_exit(-1);
}
