#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <stdint.h>

/* ============================================================
 * System Call Numbers (ABI Contract)
 * ============================================================ */
#define SYS_WRITE   0
#define SYS_EXIT    1
#define SYS_YIELD   2

/* ============================================================
 * Error Codes
 * ============================================================ */
#define E_OK        0       /* Success */
#define E_FAIL      -1      /* Generic failure */
#define E_INVAL     -2      /* Invalid syscall number */
#define E_ARG       -3      /* Invalid argument */
#define E_PTR       -4      /* Invalid pointer (User Memory Rule) */
#define E_PERM      -5      /* Permission denied */

#endif /* _SYSCALLS_H */
