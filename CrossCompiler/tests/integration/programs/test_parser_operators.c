// Test various operators
int main() {
    int a = 10;
    int b = 3;
    
    // Arithmetic
    int sum = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = a / b;
    int rem = a % b;
    
    // Comparison
    int eq = a == b;
    int ne = a != b;
    int lt = a < b;
    int gt = a > b;
    
    // Logical
    int and_result = (a > 0) && (b > 0);
    int or_result = (a > 0) || (b < 0);
    int not_result = !(a == b);
    
    // Bitwise
    int bit_and = a & b;
    int bit_or = a | b;
    int bit_xor = a ^ b;
    int lshift = a << 2;
    int rshift = a >> 1;
    
    return sum;
}
