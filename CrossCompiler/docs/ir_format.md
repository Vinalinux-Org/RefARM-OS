# IR Format Documentation

## Tổng Quan

Phase 2 Compiler sử dụng Three-Address Code (3AC) làm Intermediate Representation (IR). IR này là một low-level representation nằm giữa AST và assembly code, giúp đơn giản hóa code generation.

## Three-Address Code

### Định Nghĩa

Three-Address Code là một IR format trong đó mỗi instruction có tối đa 3 operands:

```
result = operand1 op operand2
```

### Ưu Điểm

- Simple và uniform representation
- Easy to analyze và transform
- Straightforward mapping to assembly
- Explicit temporaries cho intermediate values
- Explicit control flow với labels và jumps

## IR Instruction Types

### 1. Binary Operations

**Format**: `result = operand1 op operand2`

**Operators**:
- Arithmetic: add, sub, mul, div, mod
- Logical: and, or, xor, shl, shr
- Comparison: eq, ne, lt, gt, le, ge

**Examples**:
```
t0 = x add y
t1 = t0 mul 2
t2 = a lt b
```

### 2. Unary Operations

**Format**: `result = op operand`

**Operators**:
- neg: negation (-x)
- not: logical not (!x)
- deref: pointer dereference (*p)
- addr: address-of (&x)

**Examples**:
```
t0 = neg x
t1 = not t0
t2 = deref p
t3 = addr x
```

### 3. Assignment

**Format**: `dest = source`

**Examples**:
```
x = 5
y = t0
```

### 4. Memory Operations

**Load**: `result = load base offset`
```
t0 = load arr 4    // arr[1] (offset 4 bytes)
```

**Store**: `store base offset value`
```
store arr 0 5      // arr[0] = 5
```

### 5. Control Flow

**Label**: `label L0`
```
label L0
```

**Unconditional Jump**: `goto L0`
```
goto L0
```

**Conditional Jump**: `if_false condition goto L0`
```
if_false t0 goto L1
```

### 6. Function Operations

**Function Entry**: `function_entry name`
```
function_entry main
```

**Function Exit**: `function_exit name`
```
function_exit main
```

**Parameter**: `param value`
```
param x
param y
```

**Call**: `result = call function num_args`
```
t0 = call add 2
call print 1
```

**Return**: `return value?`
```
return 0
return t0
return
```

## IR Generation Examples

### Example 1: Simple Expression

**Source**:
```c
int x = 5 + 3 * 2;
```

**IR**:
```
t0 = 3 mul 2
t1 = 5 add t0
x = t1
```

### Example 2: If Statement

**Source**:
```c
if (x > 0) {
    y = 1;
} else {
    y = -1;
}
```

**IR**:
```
t0 = x gt 0
if_false t0 goto L1
y = 1
goto L2
label L1
y = -1
label L2
```

### Example 3: While Loop

**Source**:
```c
while (x > 0) {
    x = x - 1;
}
```

**IR**:
```
label L0
t0 = x gt 0
if_false t0 goto L1
t1 = x sub 1
x = t1
goto L0
label L1
```

### Example 4: For Loop

**Source**:
```c
for (i = 0; i < 10; i = i + 1) {
    sum = sum + i;
}
```

**IR**:
```
i = 0
label L0
t0 = i lt 10
if_false t0 goto L1
t1 = sum add i
sum = t1
t2 = i add 1
i = t2
goto L0
label L1
```

### Example 5: Function Call

**Source**:
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    return 0;
}
```

**IR**:
```
function_entry add
t0 = a add b
return t0
function_exit add

function_entry main
param 5
param 3
t1 = call add 2
result = t1
return 0
function_exit main
```

### Example 6: Array Access

**Source**:
```c
int arr[10];
arr[0] = 5;
int x = arr[1];
```

**IR**:
```
store arr 0 5
t0 = load arr 4
x = t0
```

### Example 7: Pointer Operations

**Source**:
```c
int x = 5;
int* p = &x;
*p = 10;
```

**IR**:
```
x = 5
t0 = addr x
p = t0
t1 = deref p
t1 = 10
```

## Temporaries và Labels

### Temporary Variables

- Generated bởi TempGenerator class
- Naming: t0, t1, t2, ...
- Used cho intermediate values trong expressions
- Allocated registers hoặc stack slots trong code generation

### Labels

- Generated bởi LabelGenerator class
- Naming: L0, L1, L2, ...
- Used cho control flow targets (branches, loops)
- Mapped to assembly labels trong code generation

## IR Instruction Representation

### Python Classes

Mỗi IR instruction type được represent bởi một Python class:

```python
@dataclass
class BinaryOpIR:
    op: str          # add, sub, mul, div, mod, and, or, xor, shl, shr, eq, ne, lt, gt, le, ge
    dest: str        # result variable
    left: str        # left operand
    right: str       # right operand

@dataclass
class UnaryOpIR:
    op: str          # neg, not, deref, addr
    dest: str        # result variable
    operand: str     # operand

@dataclass
class AssignIR:
    dest: str        # destination variable
    source: str      # source variable or constant

@dataclass
class LoadIR:
    dest: str        # result variable
    base: str        # base address
    offset: int      # byte offset

@dataclass
class StoreIR:
    base: str        # base address
    offset: int      # byte offset
    value: str       # value to store

@dataclass
class LabelIR:
    label: str       # label name

@dataclass
class GotoIR:
    label: str       # target label

@dataclass
class CondGotoIR:
    condition: str   # condition variable
    label: str       # target label
    is_false: bool   # jump if false (True) or true (False)

@dataclass
class ParamIR:
    value: str       # parameter value

@dataclass
class CallIR:
    dest: str        # result variable (None for void)
    function: str    # function name
    num_args: int    # number of arguments

@dataclass
class ReturnIR:
    value: str       # return value (None for void)

@dataclass
class FunctionEntryIR:
    name: str        # function name

@dataclass
class FunctionExitIR:
    name: str        # function name
```

## IR Dump Format

Khi sử dụng `--dump-ir` option, compiler outputs IR trong human-readable format:

```
=== IR ===
function_entry main
t0 = 5 add 3
x = t0
return 0
function_exit main
```

## IR Properties

### Correctness Properties

**Property 15: Three-Address Code Format**
- Mỗi instruction có tối đa 3 operands
- Binary operations: result = left op right
- Unary operations: result = op operand

**Property 16: Control Flow Label Generation**
- Mỗi label là unique
- Mỗi goto/conditional goto references một valid label
- Labels được defined trước khi referenced

**Property 17: Function Call IR Structure**
- Param instructions trước call instruction
- Number of params matches num_args trong call
- Call result assigned to temporary hoặc variable

**Property 18: Function Entry/Exit Sequences**
- Mỗi function bắt đầu với function_entry
- Mỗi function kết thúc với function_exit
- Return instructions chỉ xuất hiện trong functions

## IR Optimization (Not Implemented)

Phase 2 Compiler không implement IR optimization. Possible optimizations include:

- Constant folding
- Dead code elimination
- Common subexpression elimination
- Copy propagation
- Register allocation optimization

These optimizations có thể được added trong future phases.
