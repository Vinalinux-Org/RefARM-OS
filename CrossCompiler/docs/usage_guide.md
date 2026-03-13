# Phase 2 Compiler - Usage Guide

## Installation

### Prerequisites

**System Requirements**:
- Linux x86_64 (Ubuntu, Debian, hoặc similar)
- Python 3.8 hoặc newer
- ARM cross-compilation toolchain

**Install ARM Toolchain**:
```bash
sudo apt-get update
sudo apt-get install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
```

**Verify Installation**:
```bash
arm-linux-gnueabihf-gcc --version
arm-linux-gnueabihf-as --version
arm-linux-gnueabihf-ld --version
```

### Install Compiler

**Clone Repository**:
```bash
git clone <repository-url>
cd VinixOS
```

**Install Python Dependencies**:
```bash
cd CrossCompiler
pip3 install -r requirements.txt
```

**Build Runtime Library**:
```bash
make runtime
```

This compiles crt0.S, syscalls.S, và divmod.S thành object files.

## Basic Usage

### Compile a Program

**Basic compilation**:
```bash
python3 -m toolchain.main program.c
```

This generates executable `a.out` trong current directory.

**Specify output file**:
```bash
python3 -m toolchain.main -o myprogram program.c
```

### Run on VinixOS Platform

**Deploy to BeagleBone Black**:
```bash
./scripts/deploy_to_bbb.sh myprogram
```

**Manual deployment**:
```bash
scp myprogram debian@beaglebone.local:/home/debian/
ssh debian@beaglebone.local
./myprogram
```

## Compiler Options

### Output Control

**-o <file>**: Specify output file
```bash
python3 -m toolchain.main -o hello hello.c
```

**-S**: Generate assembly only (no assemble/link)
```bash
python3 -m toolchain.main -S -o hello.s hello.c
```

### Debug Options

**--dump-tokens**: Display tokens after lexing
```bash
python3 -m toolchain.main --dump-tokens hello.c
```

Output:
```
=== TOKENS ===
Token(type=INT, value='int', line=1, col=1)
Token(type=IDENTIFIER, value='main', line=1, col=5)
...
```

**--dump-ast**: Display AST after parsing
```bash
python3 -m toolchain.main --dump-ast hello.c
```

Output:
```
=== AST ===
Program(
  declarations=[
    FunctionDecl(name='main', ...)
  ]
)
```

**--dump-ir**: Display IR after IR generation
```bash
python3 -m toolchain.main --dump-ir hello.c
```

Output:
```
=== IR ===
function_entry main
t0 = 5 add 3
x = t0
return 0
function_exit main
```

### Help và Version

**--help**: Display help information
```bash
python3 -m toolchain.main --help
```

**--version**: Display version information
```bash
python3 -m toolchain.main --version
```

## Example Programs

### Hello World

**hello.c**:
```c
int write(int fd, char* buf, int count);
void exit(int status);

int main() {
    char msg[] = "Hello, VinixOS!\n";
    write(1, msg, 15);
    exit(0);
}
```

**Compile**:
```bash
python3 -m toolchain.main -o hello hello.c
```

### Factorial

**factorial.c**:
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

**Compile**:
```bash
python3 -m toolchain.main -o factorial factorial.c
```

### Array Operations

**array.c**:
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

**Compile**:
```bash
python3 -m toolchain.main -o array array.c
```

## Error Messages

### Lexical Errors

**Invalid character**:
```
program.c:5:10: lexical error: Invalid character '@'
```

**Unterminated character literal**:
```
program.c:3:15: lexical error: Unterminated character literal
```

### Syntax Errors

**Missing semicolon**:
```
program.c:7:5: syntax error: Expected ';', got 'return'
```

**Unmatched brace**:
```
program.c:10:1: syntax error: Expected '}', got EOF
```

### Semantic Errors

**Undeclared variable**:
```
program.c:5:5: semantic error: Undeclared variable 'x'
```

**Type mismatch**:
```
program.c:8:7: semantic error: Type mismatch in assignment: cannot assign 'char*' to 'int'
```

**Duplicate declaration**:
```
program.c:6:9: semantic error: Duplicate declaration of 'x' in same scope
```

## Build System Integration

### Makefile Integration

**Example Makefile**:
```makefile
CC = python3 -m toolchain.main
CFLAGS = 
PROGRAMS = hello factorial array

all: $(PROGRAMS)

%: %.c
	$(CC) -o $@ $<

clean:
	rm -f $(PROGRAMS) *.o *.s

.PHONY: all clean
```

**Build all programs**:
```bash
make
```

**Clean build artifacts**:
```bash
make clean
```

### Separate Compilation (Not Yet Supported)

Phase 2 Compiler hiện tại chỉ support single-file compilation. Multi-file compilation sẽ được added trong future versions.

## Testing

### Run Test Suite

**All tests**:
```bash
make test
```

**Integration tests only**:
```bash
make test-integration
```

**Unit tests only** (if available):
```bash
make test-unit
```

**Property tests only** (if available):
```bash
make test-property
```

### Test Coverage

**Generate coverage report**:
```bash
pytest --cov=toolchain --cov-report=html tests/
```

**View coverage**:
```bash
open htmlcov/index.html
```

## Debugging

### View Compilation Stages

**1. View tokens**:
```bash
python3 -m toolchain.main --dump-tokens program.c
```

**2. View AST**:
```bash
python3 -m toolchain.main --dump-ast program.c
```

**3. View IR**:
```bash
python3 -m toolchain.main --dump-ir program.c
```

**4. View assembly**:
```bash
python3 -m toolchain.main -S -o program.s program.c
cat program.s
```

### Common Issues

**Issue**: Compilation fails với "arm-linux-gnueabihf-as: command not found"

**Solution**: Install ARM toolchain:
```bash
sudo apt-get install binutils-arm-linux-gnueabihf
```

---

**Issue**: Runtime error "undefined reference to __aeabi_idiv"

**Solution**: Build runtime library:
```bash
cd CrossCompiler
make runtime
```

---

**Issue**: Program crashes on VinixOS Platform

**Solution**: 
- Verify binary format: `file myprogram` (should be ELF32 ARM)
- Verify entry point: `readelf -h myprogram` (should be 0x40000000)
- Check syscall usage (only write, read, exit, yield supported)

---

**Issue**: Semantic error "Undeclared function"

**Solution**: Add function prototype before use:
```c
int foo(int x);  // forward declaration

int main() {
    foo(5);
    return 0;
}

int foo(int x) {
    return x * 2;
}
```

## Performance Tips

### Compilation Speed

- Compiler speed is O(n) trong source code size
- Typical compilation time: < 1 second cho small programs

### Generated Code Performance

- No optimization implemented
- Code quality equivalent to GCC -O0
- Focus on correctness, not performance

### Improving Performance

Nếu cần faster execution:
1. Use GCC với optimization flags (-O2, -O3)
2. Rewrite performance-critical code trong assembly
3. Profile và optimize hot paths

## Troubleshooting

### Compiler Crashes

**Check Python version**:
```bash
python3 --version  # should be 3.8+
```

**Check dependencies**:
```bash
pip3 list | grep -E "pytest|hypothesis"
```

### Compilation Errors

**Enable verbose output**: Add print statements trong compiler code

**Check intermediate outputs**: Use --dump-tokens, --dump-ast, --dump-ir

**Simplify program**: Remove code until compilation succeeds, then add back incrementally

### Runtime Errors

**Check binary format**:
```bash
file myprogram
readelf -h myprogram
```

**Check syscalls**: Ensure only supported syscalls used (write, read, exit, yield)

**Check memory layout**: Verify base address 0x40000000

## Advanced Usage

### Custom Linker Script

Modify `toolchain/runtime/app.ld` để customize memory layout:

```ld
ENTRY(_start)

SECTIONS
{
    . = 0x40000000;  /* change base address */
    
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
```

### Custom Runtime

Modify `toolchain/runtime/crt0.S` để customize startup code:

```asm
.global _start
_start:
    ; custom initialization
    bl main
    mov r0, r0
    bl exit
```

### Adding Syscalls

1. Add syscall number trong `syscall_support.py`
2. Add syscall wrapper trong `runtime/syscalls.S`
3. Rebuild runtime library: `make runtime`

## Resources

- **Architecture Documentation**: `docs/architecture.md`
- **Subset C Specification**: `docs/subset_c_spec.md`
- **IR Format**: `docs/ir_format.md`
- **Code Generation Strategy**: `docs/codegen_strategy.md`
- **Test Programs**: `tests/integration/programs/`
- **ARM Architecture Reference**: ARMv7-A ARM
- **AAPCS Specification**: ARM Procedure Call Standard
