/*
 * reflibc.c - VinixOS Standard C Library
 * Basic I/O wrappers around VinixOS syscalls
 */

/* Internal syscall forward declaration */
int write(int fd, char* buf, int count);

/* strlen - needed internally */
int strlen(char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

/* putchar - print single character */
int putchar(int c) {
    char ch;
    ch = (char)c;
    return write(1, &ch, 1);
}

/* puts - print string + newline */
int puts(char* str) {
    int r = write(1, str, strlen(str));
    write(1, "\n", 1);
    return r;
}

/* print_str - print string (no newline) */
void print_str(char* str) {
    write(1, str, strlen(str));
}

/* print_int - print signed integer */
void print_int(int val) {
    char buf[16];
    char tmp[16];
    int i;
    int len;
    int neg;

    i = 0;
    len = 0;
    neg = 0;

    if (val == 0) {
        write(1, "0", 1);
        return;
    }
    if (val < 0) {
        neg = 1;
        val = -val;
    }
    while (val > 0) {
        tmp[i] = '0' + (val % 10);
        i = i + 1;
        val = val / 10;
    }
    if (neg) {
        buf[len] = '-';
        len = len + 1;
    }
    while (i > 0) {
        i = i - 1;
        buf[len] = tmp[i];
        len = len + 1;
    }
    write(1, buf, len);
}

/* print_hex - print integer as hex */
void print_hex(int val) {
    char buf[16];
    char tmp[16];
    char* hex;
    int i;
    int len;
    int digit;

    hex = "0123456789abcdef";
    i = 0;
    len = 0;

    if (val == 0) {
        write(1, "0", 1);
        return;
    }
    while (val > 0) {
        digit = val % 16;
        tmp[i] = hex[digit];
        i = i + 1;
        val = val / 16;
    }
    while (i > 0) {
        i = i - 1;
        buf[len] = tmp[i];
        len = len + 1;
    }
    write(1, buf, len);
}