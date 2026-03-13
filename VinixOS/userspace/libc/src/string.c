/* ============================================================
 * string.c
 * ------------------------------------------------------------
 * Userspace String Library Implementation
 * ============================================================ */

#include "string.h"

/* ============================================================
 * String Functions
 * ============================================================ */

/**
 * Compare two strings
 */
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/**
 * Copy string from src to dst
 */
void strcpy(char *dst, const char *src)
{
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
}

/**
 * Get string length
 */
int strlen(const char *s)
{
    int len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

/* ============================================================
 * Memory Functions
 * ============================================================ */

/**
 * Copy n bytes from src to dst
 */
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dst;
}

/**
 * Set n bytes of memory to value c
 */
void *memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *)s;
    
    while (n--) {
        *p++ = (uint8_t)c;
    }
    
    return s;
}

/**
 * Compare n bytes of memory
 */
int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}
