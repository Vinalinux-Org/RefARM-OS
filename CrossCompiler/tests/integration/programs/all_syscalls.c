// Test all syscalls: write, read, exit, yield
int write(int fd, char* buf, int count);
int read(int fd, char* buf, int count);
void exit(int status);
void yield();

int main() {
    char buffer[10];
    int i;
    
    // Test write syscall
    char msg[6];
    msg[0] = 84;   // 'T'
    msg[1] = 101;  // 'e'
    msg[2] = 115;  // 's'
    msg[3] = 116;  // 't'
    msg[4] = 10;   // '\n'
    msg[5] = 0;
    write(1, msg, 5);
    
    // Test yield syscall
    yield();
    
    // Test read syscall (will block if no input)
    // read(0, buffer, 10);
    
    // Test exit syscall
    exit(0);
    
    return 0;
}
