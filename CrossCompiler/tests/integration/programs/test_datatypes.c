// Test data type support
int global_int = 100;
char global_char = 65;  // 'A'
int global_array[5];

int test_pointers(int* ptr) {
    int value = *ptr;
    *ptr = value + 1;
    return *ptr;
}

int main() {
    // int (32-bit signed)
    int x = 42;
    int y = -10;
    
    // char (8-bit)
    char c1 = 'A';
    char c2 = 90;
    
    // Pointers
    int* p = &x;
    char* pc = &c1;
    
    // Arrays
    int arr[10];
    arr[0] = 5;
    arr[1] = arr[0] + 1;
    
    // Pointer arithmetic
    int result = test_pointers(&x);
    
    return result;
}
