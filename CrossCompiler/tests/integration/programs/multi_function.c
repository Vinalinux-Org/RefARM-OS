// Test multiple function calls and AAPCS calling convention
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int compute(int a, int b, int c, int d) {
    int sum1 = add(a, b);
    int sum2 = add(c, d);
    int product = multiply(sum1, sum2);
    return product;
}

int main() {
    int result = compute(1, 2, 3, 4);
    return result;
}
