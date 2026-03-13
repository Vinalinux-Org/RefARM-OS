# Kiến Trúc Phase 2 Compiler

## Tổng Quan

Phase 2 Compiler là một cross-compiler chuyển đổi mã nguồn Subset C thành binary ELF ARMv7-A. Compiler được implement bằng Python, chạy trên máy host Linux x86_64, và sinh ra code cho BeagleBone Black (Cortex-A8) chạy VinixOS.

## Pipeline Tổng Thể

```
Source Code (.c)
    ↓
[Lexer] → Tokens
    ↓
[Parser] → AST
    ↓
[Semantic Analyzer] → Annotated AST + Symbol Table
    ↓
[IR Generator] → Three-Address Code (IR)
    ↓
[Code Generator] → ARM Assembly (.s)
    ↓
[Assembler] → Object File (.o)
    ↓
[Linker] → ELF Executable
```

## Các Compiler Phases

### 1. Lexer (Phân Tích Từ Vựng)

**Module**: `compiler.frontend.lexer`

**Chức năng**: Chuyển đổi source code thành stream of tokens

**Input**: Source code text (string)

**Output**: List of tokens với type, value, line, column

**Xử lý**:
- State machine để nhận diện keywords, identifiers, literals, operators
- Track line và column numbers cho error reporting
- Bỏ qua whitespace và comments (// và /* */)
- Detect lexical errors (invalid characters, unterminated literals)

**Token Types**:
- Keywords: int, char, if, else, while, for, return, void
- Identifiers: variable và function names
- Literals: integer (decimal), character ('x')
- Operators: arithmetic, comparison, logical, bitwise, assignment
- Delimiters: (, ), {, }, [, ], ;, ,

### 2. Parser (Phân Tích Cú Pháp)

**Module**: `compiler.frontend.parser`

**Chức năng**: Chuyển đổi tokens thành Abstract Syntax Tree (AST)

**Input**: List of tokens

**Output**: AST (Program node với list of declarations)

**Xử lý**:
- Recursive descent parsing với operator precedence climbing
- Build AST nodes cho declarations, statements, expressions
- Validate syntax theo Subset C grammar
- Panic mode recovery để thu thập nhiều syntax errors

**AST Node Types**:
- Program: root node
- Declarations: FunctionDecl, VarDecl
- Statements: CompoundStmt, IfStmt, WhileStmt, ForStmt, ReturnStmt, ExprStmt
- Expressions: BinaryOp, UnaryOp, Assignment, FunctionCall, ArrayAccess, Identifier, Literals

### 3. Semantic Analyzer (Phân Tích Ngữ Nghĩa)

**Module**: `compiler.frontend.semantic`

**Chức năng**: Validate semantic correctness và build symbol table

**Input**: AST

**Output**: Annotated AST + Symbol Table

**Xử lý**:
- Traverse AST và build symbol table với scope management
- Verify tất cả variables/functions được khai báo trước khi sử dụng
- Check type compatibility trong assignments, function calls, returns
- Enforce lexical scoping rules
- Detect duplicate declarations

**Symbol Table**:
- Scope stack để track nested scopes
- Symbol entries với name, type, scope level, offset
- Function symbols với parameter types

**Type System**:
- Base types: int (32-bit), char (8-bit), void
- Pointer types: int*, char*
- Array types: treated as pointers
- Type compatibility rules với implicit conversions

### 4. IR Generator (Sinh Intermediate Representation)

**Module**: `compiler.middleend.ir`

**Chức năng**: Chuyển đổi AST thành three-address code (IR)

**Input**: Annotated AST + Symbol Table

**Output**: List of IR instructions

**Xử lý**:
- Traverse AST và sinh IR instructions
- Generate temporaries cho intermediate values
- Generate labels cho control flow
- Flatten expressions thành three-address code
- Generate function entry/exit sequences

**IR Instruction Types**:
- Arithmetic: BinaryOpIR (add, sub, mul, div, mod)
- Logical: BinaryOpIR (and, or, xor, shl, shr)
- Comparison: BinaryOpIR (eq, ne, lt, gt, le, ge)
- Unary: UnaryOpIR (neg, not, deref, addr)
- Assignment: AssignIR
- Memory: LoadIR, StoreIR
- Control flow: LabelIR, GotoIR, CondGotoIR
- Functions: ParamIR, CallIR, ReturnIR, FunctionEntryIR, FunctionExitIR

**Three-Address Code Format**:
```
result = operand1 op operand2
```

Mỗi instruction có tối đa 3 operands.

### 5. Code Generator (Sinh Mã ARMv7-A)

**Module**: `compiler.backend.armv7a`

**Chức năng**: Chuyển đổi IR thành ARM assembly

**Input**: List of IR instructions

**Output**: ARM assembly text (.s file)

**Xử lý**:
- Map IR instructions sang ARM instructions
- Allocate registers cho temporaries và variables
- Generate function prologues và epilogues theo AAPCS
- Handle register spilling khi hết registers
- Generate syscall invocations

**Register Allocation**:
- Available registers: r4-r11 (8 registers)
- r0-r3: argument passing và scratch
- r4-r11: callee-saved, used for temporaries
- sp (r13): stack pointer
- lr (r14): link register
- pc (r15): program counter

**AAPCS Calling Convention**:
- Arguments: first 4 trong r0-r3, additional trên stack
- Return value: r0
- Callee-saved: r4-r11
- Prologue: push {r4-r11, lr}, allocate stack frame
- Epilogue: deallocate stack frame, pop {r4-r11, pc}

**Instruction Selection**:
- Arithmetic: add, sub, mul, bl __aeabi_idiv (software division)
- Logical: and, orr, eor
- Shift: lsl, asr
- Comparison: cmp + conditional moves
- Memory: ldr, str với offsets
- Control flow: b, beq, bne, bgt, blt, bge, ble

**Syscall Generation**:
```asm
mov r7, #syscall_number
mov r0, arg1
mov r1, arg2
svc #0
```

### 6. Assembler

**Module**: `compiler.backend.armv7a.assembler`

**Chức năng**: Chuyển đổi assembly thành object file

**Input**: ARM assembly text (.s file)

**Output**: Object file (.o)

**Xử lý**:
- Invoke arm-linux-gnueabihf-as với -mcpu=cortex-a8
- Handle assembler errors và báo cáo

### 7. Linker

**Module**: `compiler.backend.armv7a.linker`

**Chức năng**: Link object files thành ELF executable

**Input**: Object files (.o)

**Output**: ELF executable binary

**Xử lý**:
- Invoke arm-linux-gnueabihf-ld với linker script
- Link với runtime library (crt0.o, syscalls.o, divmod.o)
- Set entry point (_start) và memory layout
- Generate ELF binary với base address 0x40000000

**Runtime Library**:
- crt0.S: C runtime startup, calls main(), exits
- syscalls.S: Syscall wrappers (write, read, exit, yield)
- divmod.S: Software division/modulo (__aeabi_idiv, __aeabi_idivmod)
- app.ld: Linker script với memory layout

**Memory Layout**:
```
0x40000000: .text (code)
0x40000000 + text_size: .rodata (read-only data)
0x40000000 + text_size + rodata_size: .data (initialized data)
0x40000000 + text_size + rodata_size + data_size: .bss (uninitialized data)
```

## Error Handling

**Error Reporting Format**:
```
<filename>:<line>:<column>: <phase> error: <message>
```

**Error Collection**:
- ErrorCollector class thu thập errors từ tất cả phases
- Continue compilation sau errors để thu thập nhiều errors
- Report tất cả errors at the end

**Exit Codes**:
- 0: Compilation success
- 1: Compilation errors

## Compiler Configuration

**CLI Options**:
- `-o <file>`: Output file path
- `-S`: Emit assembly only (no assemble/link)
- `--dump-tokens`: Dump tokens after lexing
- `--dump-ast`: Dump AST after parsing
- `--dump-ir`: Dump IR after IR generation
- `--help`: Display help information
- `--version`: Display version information

**CompilerConfig Dataclass**:
```python
@dataclass
class CompilerConfig:
    input_file: str
    output_file: str = 'a.out'
    emit_tokens: bool = False
    emit_ast: bool = False
    emit_ir: bool = False
    emit_asm: bool = False
```

## Module Organization

```
toolchain/
├── main.py                 # Compiler driver
├── common/                 # Common utilities
│   ├── error.py           # Error reporting
│   └── config.py          # Compiler configuration
├── frontend/              # Frontend phases
│   ├── lexer/            # Lexical analysis
│   │   ├── token.py      # Token definitions
│   │   └── lexer.py      # Lexer implementation
│   ├── parser/           # Syntax analysis
│   │   ├── ast_nodes.py  # AST node definitions
│   │   └── parser.py     # Parser implementation
│   └── semantic/         # Semantic analysis
│       ├── symbol_table.py  # Symbol table
│       ├── type_checker.py  # Type checking
│       └── semantic_analyzer.py  # Semantic analysis
├── middleend/            # Middle-end phases
│   └── ir/              # Intermediate representation
│       ├── ir_instructions.py  # IR instruction definitions
│       ├── generators.py      # Temp/label generators
│       └── ir_generator.py    # IR generation
├── backend/              # Backend phases
│   └── armv7a/          # ARMv7-A target
│       ├── register_allocator.py  # Register allocation
│       ├── code_generator.py      # Code generation
│       ├── syscall_support.py     # Syscall generation
│       ├── assembler.py          # Assembler wrapper
│       └── linker.py             # Linker wrapper
└── runtime/              # Runtime library
    ├── crt0.S           # C runtime startup
    ├── syscalls.S       # Syscall wrappers
    ├── divmod.S         # Software division
    └── app.ld           # Linker script
```

## Design Decisions

### 1. Python Implementation
- Rapid development và prototyping
- Rich standard library và tooling
- Easy integration với testing frameworks

### 2. Three-Address Code IR
- Simple và uniform representation
- Easy to analyze và transform
- Straightforward mapping to assembly

### 3. Software Division
- Cortex-A8 không có hardware division instruction
- Use __aeabi_idiv và __aeabi_idivmod từ runtime library
- Compatible với ARM EABI

### 4. Register Allocation
- Simple linear scan allocation
- 8 available registers (r4-r11)
- Stack spilling khi hết registers

### 5. AAPCS Calling Convention
- Standard ARM calling convention
- Compatible với C libraries
- Interoperable với other ARM code

### 6. VinixOS Platform Integration
- Base address 0x40000000 (user space)
- Syscall interface cho I/O và OS services
- ELF binary format cho loader compatibility

## Testing Strategy

### Unit Tests
- Test individual components isolation
- Test edge cases và error conditions
- Fast execution, run frequently

### Integration Tests
- Test end-to-end compilation
- Test compiler options và debug features
- Test error detection và reporting

### Property-Based Tests (Optional)
- Test universal correctness properties
- Generate random inputs với Hypothesis
- Validate invariants across all inputs

### Hardware Tests (Optional)
- Deploy to BeagleBone Black
- Execute trên VinixOS
- Verify output qua UART

## Performance Characteristics

**Compilation Speed**:
- Lexer: O(n) trong source code size
- Parser: O(n) trong token count
- Semantic: O(n) trong AST nodes
- IR Generation: O(n) trong AST nodes
- Code Generation: O(m) trong IR instructions

**Memory Usage**:
- AST: O(n) trong source code size
- Symbol Table: O(s) trong number of symbols
- IR: O(m) trong number of instructions

**Generated Code Quality**:
- No optimization (equivalent to GCC -O0)
- Simple register allocation
- Straightforward instruction selection
- Focus on correctness over performance
