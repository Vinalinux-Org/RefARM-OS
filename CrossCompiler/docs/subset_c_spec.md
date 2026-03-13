# Subset C Language Specification

## Tổng Quan

Subset C là một subset của C language được hỗ trợ bởi Phase 2 Compiler. Nó bao gồm các features cơ bản nhất của C, đủ để viết các programs đơn giản chạy trên VinixOS Platform.

## Lexical Elements

### Keywords

```
int char void if else while for return
```

### Identifiers

- Bắt đầu bằng letter hoặc underscore
- Theo sau bởi letters, digits, hoặc underscores
- Case-sensitive

```c
x
counter
_temp
myVariable
```

### Integer Literals

- Decimal integers only (no hex, octal, binary)
- Signed 32-bit range: -2147483648 to 2147483647

```c
0
42
-100
2147483647
```

### Character Literals

- Single character trong single quotes
- Escape sequences: \n, \t, \r, \\, \', \"

```c
'a'
'Z'
'0'
'\n'
'\''
```

### Operators

**Arithmetic**: `+` `-` `*` `/` `%`

**Comparison**: `==` `!=` `<` `>` `<=` `>=`

**Logical**: `&&` `||` `!`

**Bitwise**: `&` `|` `^` `<<` `>>`

**Assignment**: `=`

**Pointer**: `*` (dereference) `&` (address-of)

**Array subscript**: `[]`

### Delimiters

```
( ) { } [ ] ; ,
```

### Comments

**Single-line**: `// comment`

**Multi-line**: `/* comment */`

## Data Types

### int

- 32-bit signed integer
- Range: -2147483648 to 2147483647
- Default type cho integer literals

```c
int x;
int y = 42;
```

### char

- 8-bit character
- Used cho single characters và strings (char arrays)

```c
char c;
char ch = 'a';
```

### void

- Used cho functions không return value
- Không thể declare variables với type void

```c
void foo() {
    // no return value
}
```

### Pointers

- Pointer to int: `int*`
- Pointer to char: `char*`
- Support pointer arithmetic

```c
int* p;
char* str;
int x = 5;
p = &x;
*p = 10;
```

### Arrays

- One-dimensional arrays only
- Fixed size, declared với size
- Array name treated as pointer to first element

```c
int arr[10];
char str[20];
arr[0] = 5;
str[0] = 'H';
```

## Declarations

### Variable Declarations

```c
int x;
int y = 5;
char c = 'a';
int* p;
int arr[10];
```

### Function Declarations

```c
int add(int a, int b) {
    return a + b;
}

void print_number(int n) {
    // implementation
}

int main() {
    return 0;
}
```

**Function Prototypes** (declarations without body):
```c
int add(int a, int b);  // forward declaration
```

**Constraints**:
- Up to 4 parameters
- Parameters passed theo AAPCS (first 4 trong r0-r3)
- Return value trong r0

## Statements

### Compound Statement

```c
{
    int x = 5;
    int y = 10;
    x = x + y;
}
```

### Expression Statement

```c
x = 5;
foo();
x++;
```

### If Statement

```c
if (x > 0) {
    y = 1;
}

if (x > 0) {
    y = 1;
} else {
    y = -1;
}
```

### While Loop

```c
while (x > 0) {
    x = x - 1;
}
```

### For Loop

```c
for (i = 0; i < 10; i = i + 1) {
    sum = sum + i;
}
```

**Note**: Init, condition, và increment phải là expressions (không support declarations trong init).

### Return Statement

```c
return 0;
return x + y;
```

## Expressions

### Operator Precedence (highest to lowest)

1. Primary: `()` `[]` function call
2. Unary: `!` `-` `*` (deref) `&` (address-of)
3. Multiplicative: `*` `/` `%`
4. Additive: `+` `-`
5. Shift: `<<` `>>`
6. Relational: `<` `>` `<=` `>=`
7. Equality: `==` `!=`
8. Bitwise AND: `&`
9. Bitwise XOR: `^`
10. Bitwise OR: `|`
11. Logical AND: `&&`
12. Logical OR: `||`
13. Assignment: `=`

### Operator Associativity

- Left-to-right: binary operators (except assignment)
- Right-to-left: unary operators, assignment

### Examples

```c
// Arithmetic
x = a + b * c;
y = (a + b) * c;

// Comparison
if (x == 5 && y > 10) { }

// Logical
if (x > 0 || y < 0) { }

// Bitwise
z = x & 0xFF;
w = y << 2;

// Pointer
*p = 10;
q = &x;

// Array
arr[i] = 5;
x = arr[i + 1];

// Function call
result = add(x, y);
```

## Type System

### Type Compatibility

**Compatible assignments**:
- int = int
- char = char
- int* = int*
- char* = char*

**Implicit conversions**:
- char → int (zero-extension)
- int → char (truncation)

**Incompatible** (compile error):
- int = int*
- int* = char*
- void = anything

### Type Checking Rules

**Assignment**: Left và right sides phải compatible

**Function call**: Argument types phải match parameter types

**Return**: Return value type phải match function return type

**Array index**: Index phải là integer type

**Pointer arithmetic**: Operands phải là pointer và integer

## Scoping Rules

### Lexical Scoping

- Variables visible từ declaration point đến end of scope
- Inner scopes có thể shadow outer scope variables
- Function parameters trong function scope

### Scope Levels

- Global scope: function declarations
- Function scope: parameters và local variables
- Block scope: variables trong compound statements

```c
int x = 1;  // global

int foo(int x) {  // parameter x shadows global x
    int y = 2;    // local variable
    {
        int z = 3;  // block-local variable
        int x = 4;  // shadows parameter x
    }
    // z not visible here
    return x + y;
}
```

## Syscall Interface

### Supported Syscalls

**write(fd, buf, count)**:
- Write data to file descriptor
- Returns: number of bytes written

**read(fd, buf, count)**:
- Read data from file descriptor
- Returns: number of bytes read

**exit(status)**:
- Terminate program với exit code
- Does not return

**yield()**:
- Yield CPU to other processes
- Returns: 0

### Syscall Numbers

```c
#define SYS_write 4
#define SYS_read  3
#define SYS_exit  1
#define SYS_yield 158
```

### Example Usage

```c
int main() {
    char msg[] = "Hello, VinixOS!\n";
    write(1, msg, 15);  // write to stdout
    exit(0);
}
```

## Limitations

### Not Supported

- Floating-point types (float, double)
- Structs và unions
- Enums
- Typedefs
- Preprocessor directives (#include, #define, #ifdef)
- Multi-dimensional arrays
- String literals (use char arrays)
- Global variable initialization
- Static variables
- Const qualifier
- Function pointers
- Variadic functions
- Switch statements
- Do-while loops
- Break và continue statements
- Goto statements
- Ternary operator (?:)
- Compound assignment operators (+=, -=, etc.)
- Increment/decrement operators (++, --)

### Constraints

- Maximum 4 function parameters
- Arrays must have fixed size at declaration
- No nested function definitions
- No forward references (declare before use)

## Grammar (Simplified BNF)

```
program ::= declaration*

declaration ::= function_decl | var_decl

function_decl ::= type identifier '(' param_list? ')' (compound_stmt | ';')

param_list ::= param (',' param)*

param ::= type identifier

var_decl ::= type identifier ('[' integer ']')? ('=' expression)? ';'

type ::= 'int' | 'char' | 'void' | type '*'

compound_stmt ::= '{' (var_decl | statement)* '}'

statement ::= compound_stmt
            | 'if' '(' expression ')' statement ('else' statement)?
            | 'while' '(' expression ')' statement
            | 'for' '(' expression? ';' expression? ';' expression? ')' statement
            | 'return' expression? ';'
            | expression ';'

expression ::= assignment

assignment ::= logical_or ('=' assignment)?

logical_or ::= logical_and ('||' logical_and)*

logical_and ::= bitwise_or ('&&' bitwise_or)*

bitwise_or ::= bitwise_xor ('|' bitwise_xor)*

bitwise_xor ::= bitwise_and ('^' bitwise_and)*

bitwise_and ::= equality ('&' equality)*

equality ::= relational (('==' | '!=') relational)*

relational ::= shift (('<' | '>' | '<=' | '>=') shift)*

shift ::= additive (('<<' | '>>') additive)*

additive ::= multiplicative (('+' | '-') multiplicative)*

multiplicative ::= unary (('*' | '/' | '%') unary)*

unary ::= ('!' | '-' | '*' | '&') unary | postfix

postfix ::= primary ('[' expression ']' | '(' arg_list? ')')*

primary ::= identifier | integer | character | '(' expression ')'

arg_list ::= expression (',' expression)*
```

## Example Programs

### Hello World

```c
int write(int fd, char* buf, int count);
void exit(int status);

int main() {
    char msg[] = "Hello, VinixOS!\n";
    write(1, msg, 15);
    exit(0);
}
```

### Factorial

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    return 0;
}
```

### Array Sum

```c
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
    numbers[0] = 1;
    numbers[1] = 2;
    numbers[2] = 3;
    numbers[3] = 4;
    numbers[4] = 5;
    int total = sum_array(numbers, 5);
    return 0;
}
```
