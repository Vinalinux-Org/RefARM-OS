/* ============================================================
 * types.h
 * Basic type definitions for bootloader
 * ============================================================ */

#ifndef TYPES_H
#define TYPES_H

/* Standard integer types */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

/* Size type */
typedef unsigned int       size_t;

/* Boolean */
typedef enum {
    false = 0,
    true = 1
} bool;

/* NULL pointer */
#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* TYPES_H */