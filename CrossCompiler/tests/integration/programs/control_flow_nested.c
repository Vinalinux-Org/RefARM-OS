// Test nested control flow
int main() {
    int result = 0;
    int i;
    int j;
    
    // Nested loops
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            result = result + 1;
        }
    }
    
    // Nested if/else
    if (result > 5) {
        if (result < 15) {
            while (result < 20) {
                result = result + 1;
            }
        } else {
            result = result - 5;
        }
    }
    
    return result;
}
