# Code Generation Strategy

## Tổng Quan

Code generation phase chuyển đổi IR (three-address code) thành ARM assembly code cho ARMv7-A architecture. Phase này implement register allocation, instruction selection, và AAPCS calling convention.

## Register Allocation

### Available Registers

**Callee-Saved Registers** (r4-r11): 8 registers
- Used cho temporaries và local variables
- Must be saved/restored trong function prologue/epilogue

**Argument Registers** (r0-r3): 4 registers
- Used cho function arguments (first 4 parameters)
- Used cho return values (r0)
- Scratch registers (không cần save/restore)

**Special Registers**:
- sp (r13): Stack pointer
- lr (r14): Link register (return address)
- pc (r15): Program counter

### Allocation Strategy

1. **Linear Scan Allocation**:
   - Allocate registers on-demand cho temporaries
   - Use r4-r11 trong sequential order
   - Track allocation mapping: temp/var name → register

2. **Register Spilling**:
   - Khi hết available registers, spill least recently used register to stack
   - Store spilled value to stack slot
   - Track spilled variables: name → stack offset
   - Reload từ stack khi needed

3. **Register Freeing**:
   - Free registers sau khi temporary không còn needed
   - Reuse freed registers cho new temporaries

### Example

**IR**:
```
t0 = x add y
t1 = t0 mul 2
t2 = t1 sub z
result = t2
```

**Register Allocation**:
```
t0 → r4
t1 → r5
t2 → r6
result → r7
```

**Assembly**:
```asm
ldr r0, [fp, #-8]    ; load x
ldr r1, [fp, #-12]   ; load y
add r4, r0, r1       ; t0 = x + y
mov r0, #2
mul r5, r4, r0       ; t1 = t0 * 2
ldr r0, [fp, #-16]   ; load z
sub r6, r5, r0       ; t2 = t1 - z
str r6, [fp, #-20]   ; store result
```

## AAPCS Calling Convention

### Function Prologue

**Purpose**: Setup stack frame và save callee-saved registers

**Steps**:
1. Push callee-saved registers và link register: `push {r4-r11, lr}`
2. Setup frame pointer: `mov fp, sp` (optional)
3. Allocate stack space cho local variables: `sub sp, sp, #frame_size`

**Example**:
```asm
foo:
    push {r4-r11, lr}
    sub sp, sp, #16      ; allocate 16 bytes for locals
```

### Function Epilogue

**Purpose**: Cleanup stack frame và restore registers

**Steps**:
1. Deallocate stack space: `add sp, sp, #frame_size`
2. Pop callee-saved registers và return: `pop {r4-r11, pc}`

**Example**:
```asm
    add sp, sp, #16      ; deallocate locals
    pop {r4-r11, pc}     ; restore and return
```

### Argument Passing

**Rules**:
- First 4 arguments: r0, r1, r2, r3
- Additional arguments: push to stack (right-to-left order)

**Example** (3 arguments):
```asm
mov r0, #5           ; arg1
mov r1, #10          ; arg2
mov r2, #15          ; arg3
bl foo
```

**Example** (5 arguments):
```asm
mov r0, #1           ; arg1
mov r1, #2           ; arg2
mov r2, #3           ; arg3
mov r3, #4           ; arg4
mov r4, #5
push {r4}            ; arg5 on stack
bl foo
add sp, sp, #4       ; cleanup stack
```

### Return Value

- Return value placed trong r0
- Caller reads return value từ r0 sau function call

**Example**:
```asm
; in callee
mov r0, #42
pop {r4-r11, pc}

; in caller
bl foo
mov r4, r0           ; save return value
```

## Instruction Selection

### Arithmetic Operations

**Addition**: `add dest, left, right`
```
IR:  t0 = x add y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     add r4, r0, r1
```

**Subtraction**: `sub dest, left, right`
```
IR:  t0 = x sub y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     sub r4, r0, r1
```

**Multiplication**: `mul dest, left, right`
```
IR:  t0 = x mul y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     mul r4, r0, r1
```

**Division**: `bl __aeabi_idiv` (software division)
```
IR:  t0 = x div y
ASM: ldr r0, [fp, #x_offset]    ; dividend
     ldr r1, [fp, #y_offset]    ; divisor
     bl __aeabi_idiv
     mov r4, r0                 ; result in r0
```

**Modulo**: `bl __aeabi_idivmod` (software division + modulo)
```
IR:  t0 = x mod y
ASM: ldr r0, [fp, #x_offset]    ; dividend
     ldr r1, [fp, #y_offset]    ; divisor
     bl __aeabi_idivmod
     mov r4, r1                 ; remainder in r1
```

### Logical Operations

**AND**: `and dest, left, right`
```
IR:  t0 = x and y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     and r4, r0, r1
```

**OR**: `orr dest, left, right`
```
IR:  t0 = x or y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     orr r4, r0, r1
```

**XOR**: `eor dest, left, right`
```
IR:  t0 = x xor y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     eor r4, r0, r1
```

**Shift Left**: `lsl dest, left, right`
```
IR:  t0 = x shl y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     lsl r4, r0, r1
```

**Shift Right**: `asr dest, left, right` (arithmetic shift)
```
IR:  t0 = x shr y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     asr r4, r0, r1
```

### Comparison Operations

**Strategy**: Use cmp + conditional moves

**Equal**: `cmp + moveq`
```
IR:  t0 = x eq y
ASM: ldr r0, [fp, #x_offset]
     ldr r1, [fp, #y_offset]
     cmp r0, r1
     moveq r4, #1
     movne r4, #0
```

**Not Equal**: `cmp + movne`
```
IR:  t0 = x ne y
ASM: cmp r0, r1
     movne r4, #1
     moveq r4, #0
```

**Less Than**: `cmp + movlt`
```
IR:  t0 = x lt y
ASM: cmp r0, r1
     movlt r4, #1
     movge r4, #0
```

**Greater Than**: `cmp + movgt`
```
IR:  t0 = x gt y
ASM: cmp r0, r1
     movgt r4, #1
     movle r4, #0
```

### Unary Operations

**Negation**: `rsb dest, operand, #0`
```
IR:  t0 = neg x
ASM: ldr r0, [fp, #x_offset]
     rsb r4, r0, #0
```

**Logical NOT**: `cmp + conditional moves`
```
IR:  t0 = not x
ASM: ldr r0, [fp, #x_offset]
     cmp r0, #0
     moveq r4, #1
     movne r4, #0
```

**Dereference**: `ldr dest, [operand]`
```
IR:  t0 = deref p
ASM: ldr r0, [fp, #p_offset]
     ldr r4, [r0]
```

**Address-of**: `add dest, fp, #offset`
```
IR:  t0 = addr x
ASM: add r4, fp, #x_offset
```

### Memory Operations

**Load**: `ldr dest, [base, #offset]`
```
IR:  t0 = load arr 4
ASM: ldr r0, [fp, #arr_offset]
     ldr r4, [r0, #4]
```

**Store**: `str value, [base, #offset]`
```
IR:  store arr 0 5
ASM: ldr r0, [fp, #arr_offset]
     mov r1, #5
     str r1, [r0]
```

### Control Flow

**Unconditional Branch**: `b label`
```
IR:  goto L0
ASM: b L0
```

**Conditional Branch**: `cmp + conditional branch`
```
IR:  if_false t0 goto L1
ASM: ldr r0, [fp, #t0_offset]
     cmp r0, #0
     beq L1
```

### Function Operations

**Function Call**:
```
IR:  param 5
     param 10
     t0 = call add 2
ASM: mov r0, #5
     mov r1, #10
     bl add
     mov r4, r0
```

**Return**:
```
IR:  return t0
ASM: ldr r0, [fp, #t0_offset]
     add sp, sp, #frame_size
     pop {r4-r11, pc}
```

## Stack Frame Layout

### Structure

```
High Address
+------------------+
| Saved lr         |  <- [fp, #36]
| Saved r11        |  <- [fp, #32]
| Saved r10        |  <- [fp, #28]
| ...              |
| Saved r4         |  <- [fp, #0]
+------------------+  <- fp (frame pointer)
| Local var 1      |  <- [fp, #-4]
| Local var 2      |  <- [fp, #-8]
| ...              |
| Spilled temp     |  <- [fp, #-N]
+------------------+  <- sp (stack pointer)
Low Address
```

### Offset Calculation

- Local variables: negative offsets từ fp
- First local: [fp, #-4]
- Second local: [fp, #-8]
- Nth local: [fp, #-(N*4)]

### Frame Size

```
frame_size = num_locals * 4 + num_spilled * 4
```

## Syscall Generation

### Syscall Interface

**ARM Linux Syscall Convention**:
- Syscall number trong r7
- Arguments trong r0-r3
- Invoke với `svc #0`
- Return value trong r0

### Syscall Numbers

```
SYS_exit  = 1
SYS_read  = 3
SYS_write = 4
SYS_yield = 158
```

### Example: write(fd, buf, count)

```asm
mov r7, #4           ; SYS_write
mov r0, #1           ; fd = stdout
ldr r1, [fp, #buf]   ; buf address
mov r2, #15          ; count
svc #0               ; invoke syscall
```

### Syscall Wrappers

Runtime library cung cấp syscall wrappers trong syscalls.S:

```asm
.global write
write:
    mov r7, #4
    svc #0
    bx lr

.global read
read:
    mov r7, #3
    svc #0
    bx lr

.global exit
exit:
    mov r7, #1
    svc #0
    ; never returns

.global yield
yield:
    mov r7, #158
    svc #0
    bx lr
```

## Assembly Output Format

### Sections

**.text**: Code section
```asm
.text
.global main
main:
    ; function code
```

**.data**: Initialized data (not used trong current implementation)

**.rodata**: Read-only data (not used trong current implementation)

**.bss**: Uninitialized data (not used trong current implementation)

### GNU Assembler Syntax

- Comments: `; comment` hoặc `@ comment`
- Labels: `label_name:`
- Directives: `.text`, `.global`, `.word`
- Instructions: `mnemonic operands`

### Example Output

```asm
.text
.global main

main:
    push {r4-r11, lr}
    sub sp, sp, #8
    
    mov r0, #5
    str r0, [fp, #-4]
    
    ldr r0, [fp, #-4]
    mov r1, #3
    add r4, r0, r1
    str r4, [fp, #-8]
    
    mov r0, #0
    add sp, sp, #8
    pop {r4-r11, pc}
```

## Memory Layout và Linking

### Linker Script (app.ld)

```ld
ENTRY(_start)

SECTIONS
{
    . = 0x40000000;
    
    .text : {
        *(.text)
        *(.text.*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : {
        *(.data)
        *(.data.*)
    }
    
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
}
```

### Memory Map

```
0x40000000: _start (entry point)
0x40000000 + offset: main
0x40000000 + offset: other functions
...
```

### Runtime Library

**crt0.S**: C runtime startup
```asm
.global _start
_start:
    bl main          ; call main()
    mov r0, r0       ; main's return value already in r0
    bl exit          ; exit with return value
```

**divmod.S**: Software division
```asm
.global __aeabi_idiv
__aeabi_idiv:
    ; division implementation
    bx lr

.global __aeabi_idivmod
__aeabi_idivmod:
    ; division + modulo implementation
    bx lr
```

## Code Generation Algorithm

### Main Loop

```python
for instruction in ir_instructions:
    if isinstance(instruction, BinaryOpIR):
        generate_binary_op(instruction)
    elif isinstance(instruction, UnaryOpIR):
        generate_unary_op(instruction)
    elif isinstance(instruction, AssignIR):
        generate_assign(instruction)
    elif isinstance(instruction, LoadIR):
        generate_load(instruction)
    elif isinstance(instruction, StoreIR):
        generate_store(instruction)
    elif isinstance(instruction, LabelIR):
        emit_label(instruction.label)
    elif isinstance(instruction, GotoIR):
        emit(f"b {instruction.label}")
    elif isinstance(instruction, CondGotoIR):
        generate_cond_goto(instruction)
    elif isinstance(instruction, ParamIR):
        generate_param(instruction)
    elif isinstance(instruction, CallIR):
        generate_call(instruction)
    elif isinstance(instruction, ReturnIR):
        generate_return(instruction)
    elif isinstance(instruction, FunctionEntryIR):
        generate_function_entry(instruction)
    elif isinstance(instruction, FunctionExitIR):
        generate_function_exit(instruction)
```

### Binary Operation Generation

```python
def generate_binary_op(instr):
    # Load operands into registers
    left_reg = load_operand(instr.left)
    right_reg = load_operand(instr.right)
    
    # Allocate register for result
    dest_reg = allocate_register(instr.dest)
    
    # Emit instruction
    if instr.op == 'add':
        emit(f"add {dest_reg}, {left_reg}, {right_reg}")
    elif instr.op == 'sub':
        emit(f"sub {dest_reg}, {left_reg}, {right_reg}")
    elif instr.op == 'mul':
        emit(f"mul {dest_reg}, {left_reg}, {right_reg}")
    elif instr.op == 'div':
        emit(f"mov r0, {left_reg}")
        emit(f"mov r1, {right_reg}")
        emit(f"bl __aeabi_idiv")
        emit(f"mov {dest_reg}, r0")
    # ... other operators
    
    # Free operand registers
    free_register(left_reg)
    free_register(right_reg)
```

### Function Entry Generation

```python
def generate_function_entry(instr):
    emit(f".global {instr.name}")
    emit(f"{instr.name}:")
    emit("push {r4-r11, lr}")
    
    # Calculate frame size
    frame_size = calculate_frame_size()
    if frame_size > 0:
        emit(f"sub sp, sp, #{frame_size}")
```

### Function Exit Generation

```python
def generate_function_exit(instr):
    # Deallocate frame
    frame_size = calculate_frame_size()
    if frame_size > 0:
        emit(f"add sp, sp, #{frame_size}")
    
    emit("pop {r4-r11, pc}")
```

## Optimization Opportunities (Not Implemented)

Phase 2 Compiler không implement optimizations. Possible optimizations:

### Register Allocation
- Graph coloring allocation
- Live range analysis
- Better spilling heuristics

### Instruction Selection
- Peephole optimization
- Instruction combining
- Constant folding

### Code Layout
- Basic block reordering
- Branch prediction hints
- Function inlining

### Memory Access
- Load/store optimization
- Address calculation strength reduction
- Common subexpression elimination

## Target-Specific Considerations

### Cortex-A8 (BeagleBone Black)

**No Hardware Division**:
- Use software division routines
- __aeabi_idiv cho division
- __aeabi_idivmod cho modulo

**Instruction Set**:
- ARMv7-A instruction set
- Thumb-2 not used (ARM mode only)
- NEON not used

**Cache**:
- No cache management trong generated code
- Assume VinixOS handles cache coherency

**Alignment**:
- 4-byte alignment cho instructions
- 4-byte alignment cho word accesses
- No unaligned access support

## Assembly Generation Example

### Source Code

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    return 0;
}
```

### Generated Assembly

```asm
.text

.global add
add:
    push {r4-r11, lr}
    add r0, r0, r1
    pop {r4-r11, pc}

.global main
main:
    push {r4-r11, lr}
    sub sp, sp, #4
    
    mov r0, #5
    mov r1, #3
    bl add
    str r0, [sp, #0]
    
    mov r0, #0
    add sp, sp, #4
    pop {r4-r11, pc}
```

## Code Quality

### Correctness Focus

- Prioritize correctness over performance
- Generate straightforward, readable assembly
- Avoid complex optimizations

### Code Size

- No optimization → larger code size
- Equivalent to GCC -O0
- Acceptable cho educational purposes

### Performance

- No optimization → slower execution
- Simple register allocation
- Straightforward instruction selection
- Focus on correctness, not speed
