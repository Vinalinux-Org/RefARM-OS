// Test basic parser features
int global_var;
int array[10];

int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 5;
    int y = 10;
    int sum = add(x, y);
    
    if (sum > 10) {
        return 1;
    } else {
        return 0;
    }
}
