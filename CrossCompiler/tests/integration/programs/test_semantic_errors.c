// Test semantic error detection
int main() {
    int x = 5;
    int y = undeclared_var;  // Error: undeclared variable
    
    int z = x + y;
    
    unknown_func();  // Error: undeclared function
    
    int arr[10];
    arr[x] = 5;  // OK
    arr[&x] = 5;  // Error: array index must be integer, not pointer
    
    return z;
}
