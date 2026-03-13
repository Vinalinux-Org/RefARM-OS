/*
 * VinixOS Standard Library Implementation
 * Minimal C library functions for VinixOS Platform
 */

// Forward declarations for syscalls
int write(int fd, const char* buf, int count);

// String length function
int strlen(const char* s) {
    int len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

// Simple printf - only supports string literals (no format specifiers)
int printf(const char* str) {
    int len = strlen(str);
    return write(1, str, len);  // write to stdout (fd=1)
}

// puts function - print string with newline
int puts(const char* str) {
    int len = strlen(str);
    int result = write(1, str, len);
    write(1, "\n", 1);  // add newline
    return result;
}

// String compare
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// String copy
void strcpy(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
}

// Memory copy
void* memcpy(void* dst, const void* src, unsigned int n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dst;
}

// Memory set
void* memset(void* s, int c, unsigned int n) {
    unsigned char* p = (unsigned char*)s;
    
    while (n--) {
        *p++ = (unsigned char)c;
    }
    
    return s;
}

// Memory compare
int memcmp(const void* s1, const void* s2, unsigned int n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}