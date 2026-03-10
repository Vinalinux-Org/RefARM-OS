/* ============================================================
 * commands.c
 * ------------------------------------------------------------
 * Built-in Shell Commands (User Mode)
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

#include "syscalls.h"
#include "user_syscall.h"

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
        printf("  %s\t: %s\n", cmd->name, cmd->description);
        cmd++;
    }
    printf("\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("\nRefARM-OS Info:\n");
    printf("  OS Name  : RefARM-OS v0.1\n");
    printf("  Platform : BeagleBone Black (Cortex-A8)\n");
    printf("  Arch     : ARMv7-A\n");
    printf("  Mode     : User Mode (Restricted)\n");
    printf("  Developer: Vinalinux <vinalinux2022@gmail.com>\n");
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
    (void)argc; (void)argv;
    /* ANSI Escape Sequence for Clear Screen */
    printf("\033[2J\033[H");
    return 0;
}

static int cmd_ps(int argc, char **argv)
{
    (void)argc; (void)argv;
    process_info_t tasks[16];
    int count = sys_get_tasks(tasks, 16);
    
    if (count < 0) {
        printf("Error getting tasks\n");
        return -1;
    }
    
    printf("\nID\tNAME\t\tSTATE\n");
    printf("--\t----\t\t-----\n");
    for (int i = 0; i < count; i++) {
        const char *state_str = "UNKNOWN";
        switch(tasks[i].state) {
            case 0: state_str = "READY"; break;
            case 1: state_str = "RUNNING"; break;
            case 2: state_str = "BLOCKED"; break;
            case 3: state_str = "ZOMBIE"; break;
        }
        
        /* Formatting: ID (tab) Name (2 tabs) State */
        printf("%d\t%s\t\t%s\n", 
               tasks[i].id, tasks[i].name, state_str);
    }
    printf("\n");
    return 0;
}

static int cmd_mem(int argc, char **argv)
{
    (void)argc; (void)argv;
    mem_info_t info;
    if (sys_get_meminfo(&info) != 0) {
        printf("Error getting mem info\n");
        return -1;
    }
    
    printf("\nMemory Layout (128MB Total):\n");
    printf("  Kernel Text : %u bytes\n", info.kernel_text);
    printf("  Kernel Data : %u bytes\n", info.kernel_data);
    printf("  Kernel BSS  : %u bytes\n", info.kernel_bss);
    printf("  Kernel Stack: %u bytes\n", info.kernel_stack);
    printf("  Free Memory : %u bytes\n", info.free);
    printf("\n");
    return 0;
}

/* ============================================================
 * Command Table
 * ============================================================ */
const struct command cmd_table[] = {
    { "help",   cmd_help,   "help",         "Show this help message" },
    { "info",   cmd_info,   "info",         "Show system info" },
    { "ps",     cmd_ps,     "ps",           "Show running tasks" },
    { "mem",    cmd_mem,    "mem",          "Show memory usage" },
    { "echo",   cmd_echo,   "echo [args]",  "Echo arguments" },
    { "clear",  cmd_clear,  "clear",        "Clear screen" },
    { NULL,     NULL,       NULL,           NULL }
};