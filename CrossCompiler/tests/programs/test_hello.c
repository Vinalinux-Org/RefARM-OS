#include "reflibc.h"

int main() {
    print_str("Hello, ");
    print_str("VinixOS");
    print_str("!\n");
    
    print_str("Number: ");
    print_int(42);
    print_str("\n");
    
    print_str("Hex: 0x");
    print_hex(255);
    print_str("\n");
    
    print_str("Char: ");
    putchar('A');
    print_str("\n");

    return 0;
}
