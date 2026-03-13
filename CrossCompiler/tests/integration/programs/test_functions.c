// Test function support
int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

int mul_add(int a, int b, int c, int d) {
    return (a * b) + (c * d);
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    // Function calls with 2 parameters
    int sum = add(10, 20);
    int diff = sub(50, 30);
    
    // Function call with 4 parameters
    int result = mul_add(2, 3, 4, 5);
    
    // Recursive function call
    int fact = factorial(5);
    
    return fact;
}
