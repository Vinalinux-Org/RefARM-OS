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
    (void)argc;
    (void)argv;
    extern const struct command cmd_table[];
    const struct command *cmd = cmd_table;

    printf("\nAvailable Commands:\n");
    printf("-------------------\n");
    while (cmd->name)
    {
        printf("  %s\t: %s\n", cmd->name, cmd->description);
        cmd++;
    }
    printf("\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("\nRefARM-OS Info:\n");
    printf("  OS Name  : RefARM-OS\n");
    printf("  Platform : BeagleBone Black (Cortex-A8)\n");
    printf("  Arch     : ARMv7-A\n");
    printf("  Developer: Vinalinux <vinalinux2022@gmail.com>\n");
    printf("\n");
    return 0;
}

static int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* ANSI Escape Sequence for Clear Screen */
    printf("\033[2J\033[H");
    return 0;
}

static int cmd_ps(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    process_info_t tasks[16];
    int count = sys_get_tasks(tasks, 16);

    if (count < 0)
    {
        printf("Error getting tasks\n");
        return -1;
    }

    printf("\n%-8s%-32s%-16s\n", "ID", "NAME", "STATE");
    printf("%-8s%-32s%-16s\n", "--", "----", "-----");
    for (int i = 0; i < count; i++)
    {
        const char *state_str = "UNKNOWN";
        switch (tasks[i].state)
        {
        case 0:
            state_str = "READY";
            break;
        case 1:
            state_str = "RUNNING";
            break;
        case 2:
            state_str = "BLOCKED";
            break;
        case 3:
            state_str = "ZOMBIE";
            break;
        }

        /* Formatting: Left-aligned fixed width columns */
        printf("%-8d%-32s%-16s\n",
               tasks[i].id, tasks[i].name, state_str);
    }
    printf("\n");
    return 0;
}

static int cmd_mem(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    mem_info_t info;
    if (sys_get_meminfo(&info) != 0)
    {
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

static int cmd_ls(int argc, char **argv)
{
    const char *path = "/";
    if (argc > 1)
    {
        path = argv[1];
    }

    file_info_t files[16];
    int count = sys_listdir(path, files, 16);

    if (count < 0)
    {
        printf("Error listing directory: %d\n", count);
        return -1;
    }

    for (int i = 0; i < count; i++)
    {
        printf("  %s\n", files[i].name);
    }
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: cat <filename>\n");
        printf("Example: cat /README.txt\n");
        return -1;
    }

    const char *filename = argv[1];

    /* Open file */
    int fd = sys_open(filename, 0); /* O_RDONLY = 0 */
    if (fd < 0)
    {
        printf("Error opening file '%s': %d\n", filename, fd);
        return -1;
    }

    /* Read and display file content */
    char buffer[256];
    int bytes_read;

    printf("\n--- %s ---\n", filename);

    while ((bytes_read = sys_read_file(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0'; /* Null terminate */
        printf("%s", buffer);
    }

    if (bytes_read < 0)
    {
        printf("\nError reading file: %d\n", bytes_read);
    }

    printf("\n--- End of %s ---\n\n", filename);

    /* Close file */
    sys_close(fd);
    return 0;
}

/* ============================================================
 * Command Table
 * ============================================================ */
const struct command cmd_table[] = {
    {"help", cmd_help, "help", "Show this help message"},
    {"info", cmd_info, "info", "Show system info"},
    {"ps", cmd_ps, "ps", "Show running tasks"},
    {"mem", cmd_mem, "mem", "Show memory usage"},
    {"ls", cmd_ls, "ls [path]", "List directory contents"},
    {"cat", cmd_cat, "cat <file>", "Display file contents"},
    {"echo", cmd_echo, "echo [args]", "Echo arguments"},
    {"clear", cmd_clear, "clear", "Clear screen"},
    {NULL, NULL, NULL, NULL}};