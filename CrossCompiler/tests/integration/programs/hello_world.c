// Hello World program using write syscall
// Tests: syscall interface, string output

// Syscall declarations
int write(int fd, char* buf, int count);

int main() {
    // write(fd, buf, count)
    // fd=1 (stdout), buf=message, count=15
    char msg[15];
    
    // "Hello, VinixOS!\n"
    msg[0] = 72;   // 'H'
    msg[1] = 101;  // 'e'
    msg[2] = 108;  // 'l'
    msg[3] = 108;  // 'l'
    msg[4] = 111;  // 'o'
    msg[5] = 44;   // ','
    msg[6] = 32;   // ' '
    msg[7] = 82;   // 'R'
    msg[8] = 101;  // 'e'
    msg[9] = 102;  // 'f'
    msg[10] = 65;  // 'A'
    msg[11] = 82;  // 'R'
    msg[12] = 77;  // 'M'
    msg[13] = 33;  // '!'
    msg[14] = 10;  // '\n'
    
    write(1, msg, 15);
    
    return 0;
}
