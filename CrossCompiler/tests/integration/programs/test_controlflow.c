// Test control flow support
int main() {
    int result = 0;
    int i;
    
    // If/else
    if (result == 0) {
        result = 1;
    } else {
        result = 2;
    }
    
    // Nested if
    if (result > 0) {
        if (result < 10) {
            result = result + 1;
        }
    }
    
    // While loop
    i = 0;
    while (i < 5) {
        result = result + i;
        i = i + 1;
    }
    
    // For loop
    for (i = 0; i < 3; i = i + 1) {
        result = result * 2;
    }
    
    // Return
    return result;
}
