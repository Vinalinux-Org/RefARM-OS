/* ============================================================
 * shell.h
 * ------------------------------------------------------------
 * Interactive shell interface
 * Target: RefARM-OS Phase 1B
 * ============================================================ */

#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

/* ============================================================
 * Configuration
 * ============================================================ */

/* Maximum line length (characters) */
#define SHELL_LINE_BUFFER_SIZE  128

/* Maximum number of arguments per command */
#define SHELL_MAX_ARGS          8

/* Maximum length of single argument */
#define SHELL_MAX_ARG_LEN       32

/* ============================================================
 * Shell API
 * ============================================================ */

/**
 * Initialize shell
 * 
 * Prints welcome banner and initial prompt.
 * Must be called before shell_process_char().
 */
void shell_init(void);

/**
 * Process single character from user input
 * @param c Character received from UART
 * 
 * Handles:
 * - Printable characters: Add to line buffer and echo
 * - Enter (\r or \n): Execute command
 * - Backspace (0x08 or 0x7F): Delete character
 * - Control characters: Ignore
 * 
 * This function handles echo internally.
 */
void shell_process_char(char c);

#endif /* SHELL_H */