/* ============================================================
 * commands.c
 * ------------------------------------------------------------
 * Built-in shell commands
 * Target: RefARM-OS Phase 1B
 * ============================================================ */

#include "shell.h"
#include "uart.h"
#include "irq.h"
#include "mmio.h"
#include <stddef.h>

/* ============================================================
 * Command Structure (must match shell.c)
 * ============================================================ */

struct command {
    const char *name;
    int (*handler)(int argc, char **argv);
    const char *usage;
    const char *description;
};

/* ============================================================
 * Helper Functions
 * ============================================================ */

/**
 * Parse hexadecimal string to uint32_t
 * @param str Hex string (with or without "0x" prefix)
 * @param value Output: parsed value
 * @return 0 on success, -1 on error
 */
static int parse_hex(const char *str, uint32_t *value)
{
    const char *p = str;
    uint32_t result = 0;

    /* Skip "0x" or "0X" prefix if present */
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }

    /* Must have at least one digit after prefix */
    if (*p == '\0') {
        return -1;
    }

    /* Parse hex digits */
    while (*p != '\0') {
        char c = *p++;
        uint32_t digit;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return -1;
        }

        result = (result << 4) | digit;
    }

    *value = result;
    return 0;
}

/* ============================================================
 * Command Implementations
 * ============================================================ */

static int cmd_help(int argc, char **argv);  /* Forward declaration */

/**
 * echo - Echo arguments back
 * Usage: echo <text>
 */
static int cmd_echo(int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            uart_putc(' ');
        }
        uart_puts(argv[i]);
    }
    uart_putc('\n');

    return 0;
}

/**
 * info - Show system information
 * Usage: info
 */
static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" RefARM-OS System Information\n");
    uart_printf("========================================\n");
    uart_printf(" Platform  : BeagleBone Black\n");
    uart_printf(" SoC       : TI AM335x (Cortex-A8)\n");
    uart_printf(" Phase     : 1B - Interactive Shell\n");
    uart_printf(" Features  : UART RX Interrupt\n");
    uart_printf("           : IRQ Framework\n");
    uart_printf("           : Exception Handling\n");
    uart_printf("========================================\n");
    uart_printf("\n");

    return 0;
}

/**
 * stats - Show interrupt statistics
 * Usage: stats
 */
static int cmd_stats(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" Interrupt Statistics\n");
    uart_printf("========================================\n");
    uart_printf(" UART IRQ fires      : %u\n", uart_get_irq_fire_count());
    uart_printf(" IRQ 72 dispatches   : %u\n", irq_get_count(IRQ_UART0));
    uart_printf(" RX buffer available : %d\n", uart_rx_available());
    uart_printf("========================================\n");
    uart_printf("\n");

    return 0;
}

/**
 * mem - Read memory at address
 * Usage: mem <addr>
 */
static int cmd_mem(int argc, char **argv)
{
    uint32_t addr;
    uint32_t value;

    if (argc != 1) {
        uart_printf("Usage: mem <addr>\n");
        uart_printf("Example: mem 0x80000000\n");
        return -1;
    }

    if (parse_hex(argv[0], &addr) != 0) {
        uart_printf("Error: invalid address '%s'\n", argv[0]);
        return -1;
    }

    if (addr & 0x3) {
        uart_printf("Error: address 0x%08x not 4-byte aligned\n", addr);
        return -1;
    }

    value = mmio_read32(addr);
    uart_printf("0x%08x: 0x%08x\n", addr, value);

    return 0;
}

/**
 * clear - Clear screen
 * Usage: clear
 */
static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uart_printf("\033[2J\033[H");

    return 0;
}

/* ============================================================
 * Help Command
 * ============================================================ */

static int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    extern const struct command cmd_table[];

    uart_printf("\n");
    uart_printf("Available commands:\n");
    uart_printf("-------------------\n");

    const struct command *cmd = cmd_table;
    while (cmd->name != NULL) {
        uart_printf("  %-12s - %s\n", cmd->usage, cmd->description);
        cmd++;
    }

    uart_printf("\n");

    return 0;
}

/* ============================================================
 * Command Table
 * ============================================================ */

const struct command cmd_table[] = {
    { "help",  cmd_help,  "help",        "Show available commands" },
    { "info",  cmd_info,  "info",        "Show system information" },
    { "stats", cmd_stats, "stats",       "Show interrupt statistics" },
    { "echo",  cmd_echo,  "echo <text>", "Echo text back" },
    { "mem",   cmd_mem,   "mem <addr>",  "Read memory at address" },
    { "clear", cmd_clear, "clear",       "Clear screen" },
    { NULL,    NULL,      NULL,          NULL }
};