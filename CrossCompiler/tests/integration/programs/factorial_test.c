// Test lexer with various tokens
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

/* Multi-line
   comment test */
int main() {
    int x = 5;
    char c = 'A';
    int result = factorial(x);
    return result;
}
