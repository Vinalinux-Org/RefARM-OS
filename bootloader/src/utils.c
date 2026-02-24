/* ============================================================
 * utils.c
 * Utility functions
 * ============================================================ */

#include "boot.h"

/* ============================================================
 * Memory copy
 * ============================================================ */
void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dest;
}

/* ============================================================
 * Memory set
 * ============================================================ */
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    
    while (n--) {
        *p++ = (unsigned char)c;
    }
    
    return s;
}

/* ============================================================
 * Simple delay loop
 * ============================================================ */
void delay(uint32_t count)
{
    volatile uint32_t i;
    for (i = 0; i < count; i++);
}