/* reflibc.h - VinixOS C Runtime */

#ifndef REFLIBC_H
#define REFLIBC_H

/* Syscalls (implemented in syscalls.S) */
int  write(int fd, char* buf, int count);
int  read(int fd, char* buf, int count);
void exit(int status);
int  yield();

/* I/O wrappers (implemented in reflibc.c) */
int  strlen(char* s);
int  putchar(int c);
int  puts(char* str);
void print_str(char* str);
void print_int(int val);
void print_hex(int val);

#endif /* REFLIBC_H */
