/* main.c - Bootloader main function and boot flow */
#include "am335x.h"
#include "boot.h"
#include <stdbool.h>

/* Boot parameters structure passed to kernel */
struct boot_params {
    uint32_t reserved;
    uint32_t mem_desc_addr;  /* Pointer to memory device descriptor */
    uint8_t  boot_device;    /* Current booting device code */
    uint8_t  reset_reason;   /* Reset reason bitmask */
    uint8_t  reserved2;
    uint8_t  reserved3;
};

void panic(const char *msg)
{
    uart_puts("\r\nPANIC: ");
    uart_puts(msg);
    uart_puts("\r\nSYSTEM HALTED.\r\n");
    while (1);
}

void c_prefetch_abort_handler(void) {
    uint32_t ifsr, ifar;
    asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r" (ifsr));
    asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r" (ifar));

    uart_puts("\r\n========================================\r\n");
    uart_puts("PANIC: PREFETCH ABORT EXCEPTION!\r\n");
    uart_puts("IFSR (Instruction Fault Status): "); uart_print_hex(ifsr); uart_puts("\r\n");
    uart_puts("IFAR (Instruction Fault Address): "); uart_print_hex(ifar); uart_puts("\r\n");
    uart_puts("SYSTEM HALTED.\r\n");
    while (1);
}

void c_data_abort_handler(void) {
    uint32_t dfsr, dfar;
    asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r" (dfsr));
    asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r" (dfar));

    uart_puts("\r\n========================================\r\n");
    uart_puts("PANIC: DATA ABORT EXCEPTION!\r\n");
    uart_puts("DFAR (Fault Address): 0x"); uart_print_hex(dfar); uart_puts("\r\n");
    uart_puts("DFSR (Status): 0x"); uart_print_hex(dfsr); uart_puts("\r\n");
    uart_puts("Possible Cause: ");
    if(dfar >= 0x44E00000 && dfar <= 0x44E0FFFF) {
        uart_puts("Peripheral access without clock enable");
    } else if(dfar >= 0x4C000000 && dfar <= 0x4C000FFF) {
        uart_puts("DDR controller access before initialization");
    } else {
        uart_puts("Memory access violation");
    }
    uart_puts("\r\nSYSTEM HALTED.\r\n");
    while (1);
}

void c_undef_handler(void) {
    uint32_t ifsr, ifar;
    /* Read Instruction Fault Status and Address from CP15 */
    asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r" (ifsr));  /* IFSR */
    asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r" (ifar));  /* IFAR */

    uart_puts("\r\n========================================\r\n");
    uart_puts("PANIC: UNDEFINED INSTRUCTION!\r\n");
    uart_puts("IFAR (Fault PC): 0x"); uart_print_hex(ifar); uart_puts("\r\n");
    uart_puts("IFSR (Status):   0x"); uart_print_hex(ifsr); uart_puts("\r\n");
    uart_puts("SYSTEM HALTED.\r\n");
    while (1);
}

void bootloader_main(void)
{
    struct boot_params params;
    
    /* --------------------------------------------------------
     * STAGE 0: Enable essential clocks
     * Must be done before any peripheral access
     * -------------------------------------------------------- */
    writel(0x2, CM_PER_L4LS_CLKSTCTRL);  /* Enable L4LS clock */
    writel(0x2, CM_PER_L3_CLKSTCTRL);    /* Enable L3 clock */
    writel(0x2, CM_PER_L4FW_CLKSTCTRL);  /* Enable L4FW clock */
    delay(1000);  /* Allow clocks to stabilize */

    /* --------------------------------------------------------
     * STAGE 1: Disable Watchdog Timer 1 (WDT1)
     * ROM code enables watchdog with ~3 minute timeout
     * Disable to prevent unexpected reset during boot
     * -------------------------------------------------------- */
    writel(0xAAAA, WDT1_WSPR);
    while (readl(WDT1_WWPS) != 0);
    writel(0x5555, WDT1_WSPR);
    while (readl(WDT1_WWPS) != 0);

    /* --------------------------------------------------------
     * STAGE 2: Early UART initialization
     * -------------------------------------------------------- */
    uart_init();
    
    /* Delay for UART stabilization */
    delay(1000000);

    /* Clear screen - send newlines to clear ROM output */
    for (int i = 0; i < 10; i++) {
        uart_putc('\r');
        uart_putc('\n');
    }
    delay(100000);

    /* --------------------------------------------------------
     * STAGE 3: Print boot banner
     * -------------------------------------------------------- */
    uart_puts("========================================\r\n");
    uart_puts("RefARM-OS Bootloader\r\n");
    uart_puts("========================================\r\n");
    uart_puts("Board:  BeagleBone Black (AM335x)\r\n");
    uart_puts("UART:   Initialized @ 115200 8N1\r\n");
    uart_puts("Clock:  ROM default (48 MHz UART)\r\n");
    uart_puts("\r\n");

    /* --------------------------------------------------------
     * STAGE 4: Clock configuration (DDR PLL only)
     * -------------------------------------------------------- */
    uart_puts("Clock:  Configuring DDR PLL for 400MHz... ");
    
    /* Configure DDR PLL for 400MHz operation
     * Other PLLs (MPU, PER, CORE) already configured by ROM */
    clock_init();
    
    uart_puts("Done.\r\n");
    uart_puts("Clock:  DDR PLL locked @ 400MHz\r\n");
    uart_puts("\r\n");

    /* --------------------------------------------------------
     * STAGE 5: DDR initialization
     * -------------------------------------------------------- */
    uart_puts("DDR:    Initializing 256MB DDR3...\r\n");
    ddr_init();
    
    uart_puts("DDR:    Running memory test...\r\n");
    
    if (ddr_test() == 0) {
        uart_puts("DDR:    Test PASSED\r\n");
    } else {
        panic("DDR memory test FAILED!");
    }
    
    uart_puts("DDR:    OK (256MB @ 0x80000000)\r\n");
    uart_puts("\r\n");

    /* --------------------------------------------------------
     * STAGE 6: Initialize MMC/SD and load kernel
     * -------------------------------------------------------- */
    uart_puts("MMC:    Initializing SD card...\r\n");
    
    if (mmc_init() != 0) {
        panic("MMC initialization FAILED!");
    }
    
    uart_puts("MMC:    Initialized OK\r\n");
    
    /* Load kernel from SD card sector 2048 (offset 1MB) */
    #define KERNEL_BASE_ADDR   0x80000000
    #define KERNEL_START_SECTOR 2048
    #define KERNEL_SIZE_SECTORS 2048  /* 1MB kernel max */
    
    uart_puts("MMC:    Loading kernel from SD card...\r\n");
    
    if (mmc_read_sectors(KERNEL_START_SECTOR, KERNEL_SIZE_SECTORS, 
                        (void*)KERNEL_BASE_ADDR) != 0) {
        panic("Failed to load kernel from SD card!");
    }
    
    uart_puts("MMC:    Kernel loaded successfully\r\n");
    uart_puts("\r\n");

    /* --------------------------------------------------------
     * STAGE 7: Prepare boot parameters for kernel
     * -------------------------------------------------------- */
    uart_puts("Boot:   Setting up boot parameters...\r\n");
    
    /* Initialize boot parameters */
    params.reserved = 0;
    params.mem_desc_addr = 0;  /* TODO: Memory descriptor if available */
    params.boot_device = 0x08; /* MMC0 boot device code */
    params.reset_reason = 0x01; /* Power-on (cold) reset */
    params.reserved2 = 0;
    params.reserved3 = 0;
    
    /* --------------------------------------------------------
     * STAGE 8: Jump to kernel
     * -------------------------------------------------------- */
    uart_puts("========================================\r\n");
    uart_puts("Boot:   Jumping to kernel @ 0x80000000\r\n");
    
    /* Verify kernel image header (sanity check) */
    uint32_t *magic = (uint32_t *)0x80000000;
    uint32_t first = *magic;
    bool ok_branch = ((first & 0xFF000000) == 0xEA000000);  /* Branch instruction */
    bool ok_ldr_vec = ((first & 0xFFFFF000) == 0xE59FF000); /* LDR pc vector */
    bool ok = (ok_branch || ok_ldr_vec) && first != 0x00000000 && first != 0xFFFFFFFF;
    
    if (!ok) {
        uart_puts("KERNEL MAGIC FAIL: ");
        uart_print_hex(first);
        panic(" - Invalid kernel image!");
    }
    
    uart_puts("Kernel: Magic = ");
    uart_print_hex(first);
    uart_puts(" OK\r\n");

    uart_puts("========================================\r\n");
    uart_puts("\r\n");
    
    /* Flush UART TX FIFO before jumping to kernel */
    uart_flush();
    
    /* Jump to kernel with ARM boot parameters:
     * r0 = 0
     * r1 = machine type (BeagleBone Black = 3589)
     * r2 = pointer to boot parameters */
    asm volatile(
        "mov r0, #0\n"
        "ldr r1, =0x0E05\n"     /* BeagleBone Black MACH_TYPE (3589) */
        "mov r2, %0\n"
        "ldr pc, =0x80000000\n"
        :: "r" (&params)
    );
    
    /* Should never reach here */
    panic("Failed to jump to kernel!");
}
