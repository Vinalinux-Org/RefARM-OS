// Test loop constructs
int main() {
    int i;
    int sum = 0;
    
    // While loop
    i = 0;
    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }
    
    // For loop
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }
    
    return sum;
}
