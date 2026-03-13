/* ============================================================
 * types.h
 * ------------------------------------------------------------
 * Basic Type Definitions (Compiler-Independent)
 * ============================================================ */

#ifndef TYPES_H
#define TYPES_H

/* ============================================================
 * Integer Types
 * ============================================================ */

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

/* ============================================================
 * Size and Pointer Types
 * ============================================================ */

typedef unsigned int       size_t;
typedef signed int         ssize_t;
typedef unsigned int       uintptr_t;
typedef signed int         intptr_t;

/* ============================================================
 * Boolean Type
 * ============================================================ */

typedef enum {
    false = 0,
    true = 1
} bool;

/* ============================================================
 * NULL Pointer
 * ============================================================ */

#define NULL ((void *)0)

#endif /* TYPES_H */
