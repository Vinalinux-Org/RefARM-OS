/* ============================================================
 * shell.c
 * ------------------------------------------------------------
 * Interactive shell implementation
 * Target: RefARM-OS Phase 1B
 * ============================================================ */

#include "shell.h"
#include "uart.h"
#include <stddef.h>

/* ============================================================
 * String Helper Functions
 * ============================================================ */

/**
 * Compare two strings
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
static int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* ============================================================
 * Forward Declarations
 * ============================================================ */

/* Command handler type */
typedef int (*cmd_handler_t)(int argc, char **argv);

/* Command structure */
struct command {
    const char *name;
    cmd_handler_t handler;
    const char *usage;
    const char *description;
};

/* External command table (defined in commands.c) */
extern const struct command cmd_table[];

/* ============================================================
 * Line Buffer
 * ============================================================ */

static struct {
    char data[SHELL_LINE_BUFFER_SIZE];
    uint32_t pos;  /* Current position (0 to pos-1 has data) */
} line_buffer = { .pos = 0 };

/**
 * Initialize line buffer
 */
static void line_buffer_init(void)
{
    line_buffer.pos = 0;
    line_buffer.data[0] = '\0';
}

/**
 * Add character to line buffer
 * @param c Character to add
 * @return 0 on success, -1 if buffer full
 */
static int line_buffer_add_char(char c)
{
    if (line_buffer.pos >= SHELL_LINE_BUFFER_SIZE - 1) {
        /* Buffer full */
        return -1;
    }
    
    line_buffer.data[line_buffer.pos++] = c;
    line_buffer.data[line_buffer.pos] = '\0';
    return 0;
}

/**
 * Remove last character from line buffer
 * @return 0 on success, -1 if buffer empty
 */
static int line_buffer_backspace(void)
{
    if (line_buffer.pos == 0) {
        /* Buffer empty */
        return -1;
    }
    
    line_buffer.pos--;
    line_buffer.data[line_buffer.pos] = '\0';
    return 0;
}

/**
 * Get line buffer contents
 * @return Pointer to line buffer data
 */
static char* line_buffer_get(void)
{
    return line_buffer.data;
}

/* ============================================================
 * Command Parser
 * ============================================================ */

/**
 * Parse command line into command and arguments
 * @param line Input line (will be modified - tokenized)
 * @param cmd Output: command name
 * @param args Output: array of argument pointers
 * @param argc Output: argument count
 * @return 0 on success, -1 on error (empty line)
 */
static int parse_command(char *line, char **cmd, char **args, int *argc)
{
    char *p = line;
    
    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    
    /* Check for empty line */
    if (*p == '\0') {
        return -1;
    }
    
    /* Extract command (first token) */
    *cmd = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        p++;
    }
    
    /* Null-terminate command */
    if (*p != '\0') {
        *p++ = '\0';
    }
    
    /* Extract arguments */
    *argc = 0;
    while (*p != '\0' && *argc < SHELL_MAX_ARGS) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        
        /* Check for end of line */
        if (*p == '\0') {
            break;
        }
        
        /* Extract argument */
        args[*argc] = p;
        (*argc)++;
        
        /* Find end of argument */
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        
        /* Null-terminate argument */
        if (*p != '\0') {
            *p++ = '\0';
        }
    }
    
    return 0;
}

/* ============================================================
 * Command Dispatcher
 * ============================================================ */

/**
 * Find command in command table
 * @param name Command name
 * @return Pointer to command structure, or NULL if not found
 */
static const struct command* find_command(const char *name)
{
    const struct command *cmd = cmd_table;
    
    while (cmd->name != NULL) {
        if (strcmp(cmd->name, name) == 0) {
            return cmd;
        }
        cmd++;
    }
    
    return NULL;
}

/**
 * Execute command
 * @param line Command line (will be modified by parser)
 * @return Command exit code (0 = success)
 */
static int execute_command(char *line)
{
    char *cmd;
    char *args[SHELL_MAX_ARGS];
    int argc;
    int ret;
    
    /* Parse command line */
    ret = parse_command(line, &cmd, args, &argc);
    if (ret != 0) {
        /* Empty line - just return */
        return 0;
    }
    
    /* Find command in table */
    const struct command *command = find_command(cmd);
    if (command == NULL) {
        uart_printf("Unknown command: %s\n", cmd);
        uart_printf("Type 'help' for available commands\n");
        return -1;
    }
    
    /* Execute command handler */
    ret = command->handler(argc, args);
    
    return ret;
}

/* ============================================================
 * Shell API Implementation
 * ============================================================ */

void shell_init(void)
{
    /* Initialize line buffer */
    line_buffer_init();
    
    /* Print welcome banner */
    uart_printf("\n");
    uart_printf("========================================\n");
    uart_printf(" RefARM-OS Interactive Shell\n");
    uart_printf(" Phase 1B - Command Interface\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("Type 'help' for available commands\n");
    uart_printf("\n");
    
    /* Print initial prompt */
    uart_printf("> ");
}

void shell_process_char(char c)
{
    /* Handle Enter key - execute command */
    if (c == '\r' || c == '\n') {
        uart_printf("\n");
        
        /* Execute command */
        char *line = line_buffer_get();
        execute_command(line);
        
        /* Reset line buffer */
        line_buffer_init();
        
        /* Print new prompt */
        uart_printf("> ");
        return;
    }
    
    /* Handle Backspace/Delete */
    if (c == 0x08 || c == 0x7F) {
        if (line_buffer_backspace() == 0) {
            /* Echo backspace sequence */
            uart_putc('\b');  /* Move cursor back */
            uart_putc(' ');   /* Overwrite character */
            uart_putc('\b');  /* Move cursor back again */
        }
        /* Else: buffer empty, ignore backspace */
        return;
    }
    
    /* Handle printable characters */
    if (c >= 32 && c < 127) {
        /* Try to add to buffer */
        if (line_buffer_add_char(c) == 0) {
            /* Success - echo character */
            uart_putc(c);
        } else {
            /* Buffer full - beep */
            uart_putc('\a');  /* BEL character */
        }
        return;
    }
    
    /* Ignore other control characters */
}