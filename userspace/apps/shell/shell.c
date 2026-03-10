/* ============================================================
 * shell.c
 * ------------------------------------------------------------
 * User Mode Interactive Shell Task
 * ============================================================ */

#include "shell.h"
#include "user_syscall.h"
#include <stddef.h>
#include <stdarg.h>

/* ============================================================
 * Configuration
 * ============================================================ */
#define SHELL_LINE_BUFFER_SIZE  128

/* Map internal logging to shell helpers */
#define uart_putc shell_putc
#define uart_puts shell_puts
#define uart_printf printf


/* ============================================================
 * Output Helpers (Syscall Wrappers)
 * ============================================================ */

void shell_putc(char c)
{
    /* Ensure address is on stack */
    char buf = c;
    sys_write(&buf, 1);
}

void shell_puts(const char *s)
{
    /* 
     * Buffer on stack (valid user memory) 
     * Copying string literal to stack bypasses validation checks 
     * on .rodata and reduces syscall overhead.
     */
    char buf[64];
    int i = 0;
    
    while (*s) {
        /* Handle Newline expansion */
        if (*s == '\n') {
            /* Flush current buffer if full or before newline */
            if (i > 0) {
                sys_write(buf, i);
                i = 0;
            }
            /* Write \r */
            buf[0] = '\r';
            sys_write(buf, 1);
        }
        
        buf[i++] = *s++;
        
        /* Flush if buffer full */
        if (i >= 64) {
            sys_write(buf, i);
            i = 0;
        }
    }
    
    /* Flush remaining */
    if (i > 0) {
        sys_write(buf, i);
    }
}

/* Simple printf implementation */
static void print_uint(uint32_t num, int base)
{
    char buf[32];
    int i = 0;
    const char *digits = "0123456789abcdef";
    
    if (num == 0) {
        shell_putc('0');
        return;
    }
    
    while (num > 0) {
        buf[i++] = digits[num % base];
        num /= base;
    }
    
    while (i > 0) {
        shell_putc(buf[--i]);
    }
}

void printf(const char *fmt, ...)
{
    va_list args;
    const char *p;
    
    va_start(args, fmt);
    
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            if (*p == '\n') shell_putc('\r');
            shell_putc(*p);
            continue;
        }
        
        p++;
        if (*p == '0') { while(*++p >= '0' && *p <= '9'); }

        switch (*p) {
        case 'd':
        case 'u':
            print_uint(va_arg(args, uint32_t), 10);
            break;
        case 'x':
        case 'X':
            print_uint(va_arg(args, uint32_t), 16);
            break;
        case 's':
            {
                const char *s = va_arg(args, const char *);
                if (s) shell_puts(s);
                else shell_puts("(null)");
            }
            break;
        case 'c':
            shell_putc((char)va_arg(args, int));
            break;
        default:
            shell_putc(*p);
            break;
        }
    }
    va_end(args);
}

/* ============================================================
 * String Helper Functions
 * ============================================================ */

static int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* ============================================================
 * Command Parser & Dispatcher
 * ============================================================ */

/* Forward declaration from commands.c */
extern const struct command cmd_table[];

static int execute_command(char *line)
{
    char *args[SHELL_MAX_ARGS];
    int argc = 0;
    char *cmd = line;
    char *p = line;
    
    /* 1. Skip leading whitespace */
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0') return 0; /* Empty line */
    
    cmd = p;
    
    /* 2. Tokenize */
    while (*p != '\0' && argc < SHELL_MAX_ARGS) {
        args[argc++] = p;
        
        /* Find end of token */
        while (*p != '\0' && *p != ' ' && *p != '\t') p++;
        
        if (*p == '\0') break;
        
        /* Terminate token */
        *p++ = '\0';
        
        /* Skip to next token */
        while (*p == ' ' || *p == '\t') p++;
    }
    
    /* 3. Dispatch */
    const struct command *entry = cmd_table;
    while (entry->name != NULL) {
        if (strcmp(entry->name, cmd) == 0) {
            return entry->handler(argc, args);
        }
        entry++;
    }
    
    printf("Unknown command: %s\n", cmd);
    printf("Type 'help' for available commands\n");
    return -1;
}

/* ============================================================
 * Line Buffer
 * ============================================================ */
static struct {
    char data[SHELL_LINE_BUFFER_SIZE];
    uint32_t pos;
} line_buffer;

static void line_buffer_init(void) {
    line_buffer.pos = 0;
    line_buffer.data[0] = '\0';
}

static int line_buffer_add(char c) {
    if (line_buffer.pos >= SHELL_LINE_BUFFER_SIZE - 1) return -1;
    line_buffer.data[line_buffer.pos++] = c;
    line_buffer.data[line_buffer.pos] = '\0';
    return 0;
}

static int line_buffer_backspace(void) {
    if (line_buffer.pos > 0) {
        line_buffer.pos--;
        line_buffer.data[line_buffer.pos] = '\0';
        return 0;
    }
    return -1;
}

/* ============================================================
 * Shell Main Entry
 * ============================================================ */
int main(void)
{
    char c;
    
    /* 1. Initialization */
    line_buffer_init();
    
    printf("\n");
    printf("========================================\n");
    printf(" RefARM-OS User Shell\n");
    printf("========================================\n");
    
    /* DEBUG: Check CPSR Mode */
    uint32_t cpsr;
    __asm__ __volatile__("mrs %0, cpsr" : "=r" (cpsr));
    printf("[SHELL] CPSR=0x%x, Mode=0x%x (USR=0x10, SVC=0x13)\n", cpsr, cpsr & 0x1F);
    
    printf("> ");
    
    /* 2. Main Loop */
    while (1) {
        /* Non-Blocking Read */
        int result = sys_read(&c, 1);
        
        if (result > 0) {
            /* Got data - process it */
            
            /* Handle Entry */
            if (c == '\r' || c == '\n') {
                shell_putc('\n');
                execute_command(line_buffer.data);
                line_buffer_init();
                printf("> ");
            }
            /* Handle Backspace (ASCII 8 or 127) */
            else if (c == 0x08 || c == 0x7F) {
                if (line_buffer_backspace() == 0) {
                    /* Visual Backspace */
                    shell_putc('\b');
                    shell_putc(' ');
                    shell_putc('\b');
                }
            }
            /* Handle Printable Chars */
            else if (c >= 32 && c <= 126) {
                if (line_buffer_add(c) == 0) {
                    shell_putc(c); /* Local Echo */
                } else {
                    shell_putc('\a'); /* Beep */
                }
            }
        } else {
            /* No data available - yield to other tasks */
            sys_yield();
        }
    }
    
    return 0;
}