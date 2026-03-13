// Test array operations
int sum_array(int* arr, int size) {
    int sum = 0;
    int i;
    for (i = 0; i < size; i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
}

int main() {
    int numbers[5];
    int i;
    
    // Initialize array
    for (i = 0; i < 5; i = i + 1) {
        numbers[i] = i * 2;
    }
    
    // Read from array
    int first = numbers[0];
    int last = numbers[4];
    
    // Sum array
    int total = sum_array(numbers, 5);
    
    return total;
}
