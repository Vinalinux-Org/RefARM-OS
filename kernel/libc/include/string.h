/* ============================================================
 * string.h
 * ------------------------------------------------------------
 * Kernel String Library
 * ============================================================ */

#ifndef STRING_H
#define STRING_H

#include "types.h"

/* ============================================================
 * String Functions
 * ============================================================ */

/**
 * Compare two strings
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strcmp(const char *s1, const char *s2);

/**
 * Copy string from src to dst (including null terminator)
 */
void strcpy(char *dst, const char *src);

/**
 * Get string length (excluding null terminator)
 */
int strlen(const char *s);

/* ============================================================
 * Memory Functions
 * ============================================================ */

/**
 * Copy n bytes from src to dst
 */
void *memcpy(void *dst, const void *src, size_t n);

/**
 * Set n bytes of memory to value c
 */
void *memset(void *s, int c, size_t n);

/**
 * Compare n bytes of memory
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int memcmp(const void *s1, const void *s2, size_t n);

#endif /* STRING_H */
