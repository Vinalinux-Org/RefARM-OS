/* ============================================================
 * commands.c
 * ------------------------------------------------------------
 * Built-in Shell Commands (User Mode)
 * Target: RefARM-OS Phase 4
 * ============================================================ */

#include "shell.h"
#include <stddef.h>

/* Map internal logging to shell helpers within Shell Task */
extern void printf(const char *fmt, ...);
void shell_putc(char c);
void shell_puts(const char *s);
#define uart_putc shell_putc
#define uart_puts shell_puts
#define uart_printf printf

/* ============================================================
 * Command Implementations
 * ============================================================ */

static int cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    extern const struct command cmd_table[];
    const struct command *cmd = cmd_table;
    
    printf("\nAvailable Commands:\n");
    printf("-------------------\n");
    while (cmd->name) {
        printf("  %s : %s\n", cmd->name, cmd->description);
        cmd++;
    }
    printf("\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    printf("\nRefARM-OS Info:\n");
    printf("  Platform : BeagleBone Black (Cortex-A8)\n");
    printf("  Mode     : User Mode (Restricted)\n");
    printf("  Shell    : Built-in Simple Shell\n");
    printf("\n");
    return 0;
}

static int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_clear(int argc, char **argv)
{
    /* ANSI Escape Sequence for Clear Screen */
    printf("\033[2J\033[H");
    return 0;
}

/* ============================================================
 * Command Table
 * ============================================================ */
const struct command cmd_table[] = {
    { "help",   cmd_help,   "help",         "Show this help message" },
    { "info",   cmd_info,   "info",         "Show system info" },
    { "echo",   cmd_echo,   "echo [args]",  "Echo arguments" },
    { "clear",  cmd_clear,  "clear",        "Clear screen" },
    { NULL,     NULL,       NULL,           NULL }
};