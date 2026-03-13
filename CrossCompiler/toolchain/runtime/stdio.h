#ifndef STDIO_H
#define STDIO_H

// Simple stdio.h for VinixOS Platform
// Provides basic I/O functions using VinixOS syscalls

// Forward declarations
int printf(const char* str);
int puts(const char* str);

// Syscall declarations (from VinixOS)
int write(int fd, const char* buf, int count);
int read(int fd, char* buf, int count);
void exit(int status);
int yield(void);

#endif /* STDIO_H */