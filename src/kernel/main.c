/* ============================================================
 * kernel/main.c
 * ------------------------------------------------------------
 * Kernel entry point with exception handler tests
 * Target: AM335x / BeagleBone Black
 * ============================================================ */

#include "uart.h"
#include "watchdog.h"
#include "exception.h"  /* Exception handler prototypes (for completeness) */

/* ============================================================
 * Test Configuration
 * ------------------------------------------------------------
 * Uncomment ONE test at a time to test different exceptions
 * ============================================================ */

/* Test options - uncomment ONE at a time */
 #define TEST_UNDEFINED_INSTRUCTION    // Test 1: Undefined instruction
// #define TEST_DATA_ABORT               // Test 2: Data abort (bad address)
// #define TEST_DATA_ABORT_ALIGNMENT     // Test 3: Data abort (alignment)
// #define TEST_PREFETCH_ABORT           // Test 4: Prefetch abort (jump to bad address)
// #define TEST_R0_PRESERVATION          // Test 5: Verify r0 preservation

/* ============================================================
 * Exception Test Functions
 * ============================================================ */

#ifdef TEST_UNDEFINED_INSTRUCTION
/**
 * Test 1: Undefined Instruction Exception
 * Triggers: Invalid opcode
 * Expected: Exception handler prints diagnostics and halts
 */
static void test_undefined_instruction(void)
{
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" TEST 1: Undefined Instruction\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("About to execute invalid opcode 0xFFFFFFFF...\n");
    uart_printf("Expected: Exception handler should catch this\n");
    uart_printf("\n");
    
    /* Execute invalid instruction */
    asm volatile(".word 0xFFFFFFFF");
    
    /* Should NEVER reach here */
    uart_printf("ERROR: Exception handler did not halt!\n");
}
#endif

#ifdef TEST_DATA_ABORT
/**
 * Test 2: Data Abort Exception (Write to Read-Only Memory)
 * Triggers: Writing to Boot ROM (read-only region)
 * Expected: Exception handler prints diagnostics and halts
 * 
 * NOTE: AM335x Boot ROM at 0x40000000 is read-only.
 * Writing here should trigger Data Abort on most ARM systems.
 * 
 * Alternative test addresses if this doesn't work:
 *   - 0x00000000 (NULL pointer)
 *   - 0x50000000 (reserved peripheral space)
 */
static void test_data_abort(void)
{
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" TEST 2: Data Abort (Write to ROM)\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("About to write to Boot ROM at 0x40000000...\n");
    uart_printf("(Boot ROM is read-only)\n");
    uart_printf("Expected: Data abort exception\n");
    uart_printf("\n");
    
    /* Write to Boot ROM (read-only) */
    volatile uint32_t *rom_ptr = (volatile uint32_t*)0x40000000;
    *rom_ptr = 0x42;
    
    /* If we reach here, try NULL pointer as fallback */
    uart_printf("Note: ROM write did not trigger exception.\n");
    uart_printf("Trying NULL pointer dereference...\n");
    
    volatile uint32_t *null_ptr = (volatile uint32_t*)0x00000000;
    *null_ptr = 0x42;
    
    /* Should NEVER reach here */
    uart_printf("ERROR: No data abort triggered!\n");
    uart_printf("This may be expected on some ARM configurations with MMU OFF.\n");
}
#endif

#ifdef TEST_DATA_ABORT_ALIGNMENT
/**
 * Test 3: Data Abort Exception (Alignment Fault)
 * Triggers: Unaligned memory access (if alignment checking enabled)
 * Expected: Data abort exception (on some ARM configurations)
 * 
 * NOTE: Cortex-A8 may handle unaligned access in hardware
 * This test may NOT trigger exception on all configurations
 */
static void test_alignment_fault(void)
{
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" TEST 3: Data Abort (Alignment)\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("About to perform unaligned 32-bit write...\n");
    uart_printf("Expected: May trigger data abort (config-dependent)\n");
    uart_printf("\n");
    
    /* Unaligned write (address not 4-byte aligned) */
    volatile uint32_t *unaligned_ptr = (volatile uint32_t*)0x80000001;
    *unaligned_ptr = 0x12345678;
    
    /* May or may not reach here depending on configuration */
    uart_printf("Note: Unaligned access completed (no exception)\n");
    uart_printf("Cortex-A8 may handle this in hardware.\n");
}
#endif

#ifdef TEST_PREFETCH_ABORT
/**
 * Test 4: Prefetch Abort Exception
 * Triggers: Jumping to invalid memory address
 * Expected: Prefetch abort exception
 */
static void test_prefetch_abort(void)
{
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" TEST 4: Prefetch Abort\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("About to jump to invalid address 0xDEADBEEF...\n");
    uart_printf("Expected: Prefetch abort exception\n");
    uart_printf("\n");
    
    /* Jump to invalid address */
    void (*bad_func)(void) = (void (*)(void))0xDEADBEEF;
    bad_func();
    
    /* Should NEVER reach here */
    uart_printf("ERROR: Exception handler did not halt!\n");
}
#endif

#ifdef TEST_R0_PRESERVATION
/**
 * Test 5: Verify r0 Preservation
 * Purpose: Verify exception handler preserves r0 correctly
 * Expected: Exception handler should print correct r0 value
 */
static void test_r0_preservation(void)
{
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" TEST 5: r0 Preservation\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("Setting r0 to 0xDEADBEEF before exception...\n");
    uart_printf("Expected: Exception handler should show r0 = 0xDEADBEEF\n");
    uart_printf("\n");
    
    /* 
     * Load test value into r0 and trigger exception.
     * 
     * We use inline assembly to ensure r0 contains the test
     * value at the moment the exception occurs.
     * 
     * The exception handler should preserve and display this
     * exact value, verifying the context save fix is correct.
     */
    asm volatile(
        "mov r0, #0xDE\n"
        "orr r0, r0, #0xAD00\n"
        "orr r0, r0, #0xBE0000\n"
        "orr r0, r0, #0xEF000000\n"  /* r0 = 0xDEADBEEF */
        ".word 0xFFFFFFFF\n"         /* Trigger undefined instruction */
        : /* no outputs */
        : /* no inputs */
        : "r0"  /* r0 will be clobbered by mov instructions */
    );
    
    /* Should NEVER reach here */
    uart_printf("ERROR: Exception handler did not halt!\n");
}
#endif

/* ============================================================
 * Main Kernel Entry Point
 * ============================================================ */

void kernel_main(void)
{
    /*
     * CRITICAL: Disable watchdog FIRST
     * AM335x boots with watchdog enabled (~3 minute timeout)
     * Must disable before any other operations
     */
    watchdog_disable();
    
    /* Initialize UART for debug output */
    uart_init();
    
    /* ========================================
     * Exception Handler Tests
     * ========================================
     * Uncomment ONE test at a time in the
     * #define section at top of file
     * ======================================== */
    
#ifdef TEST_UNDEFINED_INSTRUCTION
    test_undefined_instruction();
#endif

#ifdef TEST_DATA_ABORT
    test_data_abort();
#endif

#ifdef TEST_DATA_ABORT_ALIGNMENT
    test_alignment_fault();
#endif

#ifdef TEST_PREFETCH_ABORT
    test_prefetch_abort();
#endif

#ifdef TEST_R0_PRESERVATION
    test_r0_preservation();
#endif

    /* If no test defined, just halt normally */
#if !defined(TEST_UNDEFINED_INSTRUCTION) && \
    !defined(TEST_DATA_ABORT) && \
    !defined(TEST_DATA_ABORT_ALIGNMENT) && \
    !defined(TEST_PREFETCH_ABORT) && \
    !defined(TEST_R0_PRESERVATION)
    
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" No exception test enabled\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("To test exception handlers:\n");
    uart_printf("1. Edit src/kernel/main.c\n");
    uart_printf("2. Uncomment ONE test #define\n");
    uart_printf("3. Rebuild and deploy\n");
    uart_printf("\n");
    uart_printf("Available tests:\n");
    uart_printf("  - TEST_UNDEFINED_INSTRUCTION\n");
    uart_printf("  - TEST_DATA_ABORT\n");
    uart_printf("  - TEST_DATA_ABORT_ALIGNMENT\n");
    uart_printf("  - TEST_PREFETCH_ABORT\n");
    uart_printf("  - TEST_R0_PRESERVATION\n");
    uart_printf("\n");
    
#endif
    
    /* Normal kernel halt */
    uart_printf("Kernel halting.\n");
    while (1)
        ;
}