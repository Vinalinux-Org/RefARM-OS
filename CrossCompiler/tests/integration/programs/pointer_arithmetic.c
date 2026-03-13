// Test pointer arithmetic
int main() {
    int arr[5];
    int* ptr;
    int i;
    
    // Initialize array
    for (i = 0; i < 5; i = i + 1) {
        arr[i] = i * 10;
    }
    
    // Pointer to array
    ptr = arr;
    
    // Pointer arithmetic
    int first = *ptr;           // arr[0]
    int second = *(ptr + 1);    // arr[1]
    
    // Pointer increment
    ptr = ptr + 2;
    int third = *ptr;           // arr[2]
    
    // Address arithmetic
    int* addr1 = &arr[0];
    int* addr2 = &arr[4];
    
    return third;
}
