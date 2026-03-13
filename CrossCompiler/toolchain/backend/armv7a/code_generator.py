"""
ARMv7-A Code Generator
Generates ARM assembly from IR
"""

from typing import List, Dict, Set
from ...middleend.ir.ir_instructions import *
from .register_allocator import RegisterAllocator
from .syscall_support import is_syscall, generate_syscall


class CodeGenerator:
    """
    Code generator for ARMv7-A
    
    Generates GNU assembler syntax ARM assembly from IR.
    Implements AAPCS calling convention.
    """
    
    def __init__(self):
        """Initialize code generator"""
        self.assembly: List[str] = []
        self.register_allocator = RegisterAllocator()
        self.current_function = None
        self.local_vars: Dict[str, int] = {}  # variable name → stack offset
        self.stack_size = 0
        self.pending_params: List[str] = []  # Track params for function calls
    
    def generate(self, instructions: List[IRInstruction]) -> str:
        """
        Generate assembly from IR instructions
        
        Args:
            instructions: List of IR instructions
        
        Returns:
            Assembly code as string
        """
        self.assembly = []
        
        # Emit assembly header
        self.emit_header()
        
        # Process instructions
        for instr in instructions:
            self.visit_instruction(instr)
        
        return '\n'.join(self.assembly)
    
    def emit(self, line: str) -> None:
        """Add line to assembly output"""
        self.assembly.append(line)
    
    def emit_header(self) -> None:
        """Emit assembly file header"""
        self.emit(".syntax unified")
        self.emit(".arch armv7-a")
        self.emit(".text")
        self.emit("")
    
    # ============================================================
    # Instruction Visitors
    # ============================================================
    
    def visit_instruction(self, instr: IRInstruction) -> None:
        """Visit IR instruction and generate assembly"""
        if isinstance(instr, FunctionEntryIR):
            self.visit_function_entry(instr)
        elif isinstance(instr, FunctionExitIR):
            self.visit_function_exit(instr)
        elif isinstance(instr, BinaryOpIR):
            self.visit_binary_op(instr)
        elif isinstance(instr, UnaryOpIR):
            self.visit_unary_op(instr)
        elif isinstance(instr, AssignIR):
            self.visit_assign(instr)
        elif isinstance(instr, LoadIR):
            self.visit_load(instr)
        elif isinstance(instr, StoreIR):
            self.visit_store(instr)
        elif isinstance(instr, LabelIR):
            self.visit_label(instr)
        elif isinstance(instr, GotoIR):
            self.visit_goto(instr)
        elif isinstance(instr, CondGotoIR):
            self.visit_cond_goto(instr)
        elif isinstance(instr, ParamIR):
            self.visit_param(instr)
        elif isinstance(instr, CallIR):
            self.visit_call(instr)
        elif isinstance(instr, ReturnIR):
            self.visit_return(instr)
        elif isinstance(instr, StringLiteralIR):
            self.visit_string_literal(instr)
    
    # ============================================================
    # Function Entry/Exit (AAPCS Prologue/Epilogue)
    # ============================================================
    
    def visit_function_entry(self, instr: FunctionEntryIR) -> None:
        """
        Generate function prologue
        
        AAPCS prologue:
        1. Push callee-saved registers (r4-r11, lr)
        2. Allocate stack frame for locals
        """
        self.current_function = instr.name
        self.register_allocator.reset()
        self.local_vars = {}
        self.stack_size = 0
        
        # Emit function label
        self.emit(f".global {instr.name}")
        self.emit(f".type {instr.name}, %function")
        self.emit(f"{instr.name}:")
        
        # Prologue will be emitted at function exit when we know stack size
        self.emit("    @ Function prologue")
        self.emit("    push {r4-r11, lr}")
        
        # Reserve space for stack frame (will be calculated later)
        # For now, emit placeholder
        self.emit("    @ Stack frame allocation (placeholder)")
    
    def visit_function_exit(self, instr: FunctionExitIR) -> None:
        """
        Generate function epilogue
        
        AAPCS epilogue:
        1. Deallocate stack frame
        2. Pop callee-saved registers and return (pop {r4-r11, pc})
        """
        self.emit("")
        self.emit(f".{instr.name}_epilogue:")
        self.emit("    @ Function epilogue")
        
        # Deallocate stack frame if needed
        if self.stack_size > 0:
            self.emit(f"    add sp, sp, #{self.stack_size}")
        
        # Pop registers and return
        self.emit("    pop {r4-r11, pc}")
        self.emit("")
    
    # ============================================================
    # Arithmetic and Logic Operations
    # ============================================================
    
    def visit_binary_op(self, instr: BinaryOpIR) -> None:
        """Generate code for binary operation"""
        # Get operand registers/values
        left_reg = self.get_operand(instr.left)
        right_reg = self.get_operand(instr.right)
        result_reg = self.register_allocator.allocate(instr.result)
        
        # Generate instruction based on operator
        if instr.operator == '+':
            self.emit(f"    add {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '-':
            self.emit(f"    sub {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '*':
            self.emit(f"    mul {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '/':
            # Division: call __aeabi_idiv (software division)
            # Save r0-r3 if needed
            self.emit(f"    mov r0, {left_reg}")
            self.emit(f"    mov r1, {right_reg}")
            self.emit(f"    bl __aeabi_idiv")
            self.emit(f"    mov {result_reg}, r0")
        elif instr.operator == '%':
            # Modulo: call __aeabi_idivmod (returns quotient in r0, remainder in r1)
            self.emit(f"    mov r0, {left_reg}")
            self.emit(f"    mov r1, {right_reg}")
            self.emit(f"    bl __aeabi_idivmod")
            self.emit(f"    mov {result_reg}, r1")
        
        # Comparison operators
        elif instr.operator == '==':
            self.emit(f"    cmp {left_reg}, {right_reg}")
            self.emit(f"    moveq {result_reg}, #1")
            self.emit(f"    movne {result_reg}, #0")
        elif instr.operator == '!=':
            self.emit(f"    cmp {left_reg}, {right_reg}")
            self.emit(f"    movne {result_reg}, #1")
            self.emit(f"    moveq {result_reg}, #0")
        elif instr.operator == '<':
            self.emit(f"    cmp {left_reg}, {right_reg}")
            self.emit(f"    movlt {result_reg}, #1")
            self.emit(f"    movge {result_reg}, #0")
        elif instr.operator == '>':
            self.emit(f"    cmp {left_reg}, {right_reg}")
            self.emit(f"    movgt {result_reg}, #1")
            self.emit(f"    movle {result_reg}, #0")
        elif instr.operator == '<=':
            self.emit(f"    cmp {left_reg}, {right_reg}")
            self.emit(f"    movle {result_reg}, #1")
            self.emit(f"    movgt {result_reg}, #0")
        elif instr.operator == '>=':
            self.emit(f"    cmp {left_reg}, {right_reg}")
            self.emit(f"    movge {result_reg}, #1")
            self.emit(f"    movlt {result_reg}, #0")
        
        # Logical operators
        elif instr.operator == '&&':
            # Logical AND: both must be non-zero
            self.emit(f"    cmp {left_reg}, #0")
            self.emit(f"    movne {result_reg}, #1")
            self.emit(f"    moveq {result_reg}, #0")
            self.emit(f"    cmp {right_reg}, #0")
            self.emit(f"    moveq {result_reg}, #0")
        elif instr.operator == '||':
            # Logical OR: at least one must be non-zero
            self.emit(f"    orr {result_reg}, {left_reg}, {right_reg}")
            self.emit(f"    cmp {result_reg}, #0")
            self.emit(f"    movne {result_reg}, #1")
        
        # Bitwise operators
        elif instr.operator == '&':
            self.emit(f"    and {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '|':
            self.emit(f"    orr {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '^':
            self.emit(f"    eor {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '<<':
            self.emit(f"    lsl {result_reg}, {left_reg}, {right_reg}")
        elif instr.operator == '>>':
            self.emit(f"    asr {result_reg}, {left_reg}, {right_reg}")
    
    def visit_unary_op(self, instr: UnaryOpIR) -> None:
        """Generate code for unary operation"""
        operand_reg = self.get_operand(instr.operand)
        result_reg = self.register_allocator.allocate(instr.result)
        
        if instr.operator == '-':
            # Negation: result = 0 - operand
            self.emit(f"    rsb {result_reg}, {operand_reg}, #0")
        elif instr.operator == '!':
            # Logical NOT
            self.emit(f"    cmp {operand_reg}, #0")
            self.emit(f"    moveq {result_reg}, #1")
            self.emit(f"    movne {result_reg}, #0")
        elif instr.operator == '&':
            # Address-of (handled in semantic analysis)
            self.emit(f"    @ Address-of operator")
            self.emit(f"    mov {result_reg}, {operand_reg}")
    
    def visit_assign(self, instr: AssignIR) -> None:
        """Generate code for assignment"""
        source_reg = self.get_operand(instr.source)
        dest_reg = self.register_allocator.allocate(instr.dest)
        
        if source_reg != dest_reg:
            self.emit(f"    mov {dest_reg}, {source_reg}")
    
    # ============================================================
    # Memory Operations
    # ============================================================
    
    def visit_load(self, instr: LoadIR) -> None:
        """Generate code for load from memory"""
        addr_reg = self.get_operand(instr.address)
        result_reg = self.register_allocator.allocate(instr.result)
        
        self.emit(f"    ldr {result_reg}, [{addr_reg}]")
    
    def visit_store(self, instr: StoreIR) -> None:
        """Generate code for store to memory"""
        addr_reg = self.get_operand(instr.address)
        value_reg = self.get_operand(instr.value)
        
        self.emit(f"    str {value_reg}, [{addr_reg}]")
    
    # ============================================================
    # Control Flow
    # ============================================================
    
    def visit_label(self, instr: LabelIR) -> None:
        """Generate label"""
        self.emit(f".{self.current_function}_{instr.label}:")
    
    def visit_goto(self, instr: GotoIR) -> None:
        """Generate unconditional branch"""
        self.emit(f"    b .{self.current_function}_{instr.label}")
    
    def visit_cond_goto(self, instr: CondGotoIR) -> None:
        """Generate conditional branch"""
        cond_reg = self.get_operand(instr.condition)
        
        # Compare condition with 0
        self.emit(f"    cmp {cond_reg}, #0")
        
        if instr.is_true:
            # Branch if non-zero (true)
            self.emit(f"    bne .{self.current_function}_{instr.label}")
        else:
            # Branch if zero (false)
            self.emit(f"    beq .{self.current_function}_{instr.label}")
    
    # ============================================================
    # Function Calls (AAPCS)
    # ============================================================
    
    def visit_param(self, instr: ParamIR) -> None:
        """
        Generate code for function parameter
        
        AAPCS: First 4 args in r0-r3, rest on stack
        """
        # Collect parameters for next call
        self.pending_params.append(instr.value)
    
    def visit_call(self, instr: CallIR) -> None:
        """
        Generate code for function call
        
        AAPCS calling convention:
        - First 4 arguments in r0-r3
        - Additional arguments on stack
        - Return value in r0
        """
        # Move arguments to r0-r3
        arg_regs = ['r0', 'r1', 'r2', 'r3']
        for i, param in enumerate(self.pending_params[:4]):
            param_reg = self.get_operand(param)
            if param_reg != arg_regs[i]:
                self.emit(f"    mov {arg_regs[i]}, {param_reg}")
        
        # Clear pending params
        self.pending_params = []
        
        # Check if syscall
        if is_syscall(instr.function):
            # Generate syscall instructions
            syscall_instrs = generate_syscall(instr.function, arg_regs[:instr.num_args])
            for asm_instr in syscall_instrs:
                self.emit(asm_instr)
        else:
            # Regular function call
            self.emit(f"    bl {instr.function}")
        
        # Move return value to result register
        if instr.result:
            result_reg = self.register_allocator.allocate(instr.result)
            if result_reg != 'r0':
                self.emit(f"    mov {result_reg}, r0")
    
    def visit_return(self, instr: ReturnIR) -> None:
        """
        Generate code for return statement
        
        AAPCS: Return value in r0
        """
        if instr.value:
            value_reg = self.get_operand(instr.value)
            if value_reg != 'r0':
                self.emit(f"    mov r0, {value_reg}")
        
        # Jump to epilogue
        self.emit(f"    b .{self.current_function}_epilogue")
    
    def visit_string_literal(self, instr: StringLiteralIR) -> None:
        """
        Generate code for string literal
        
        Creates string in .rodata section
        """
        # Switch to read-only data section
        self.emit(".section .rodata")
        self.emit(f"{instr.label}:")
        
        # Emit string with null terminator
        escaped_content = instr.content.replace('\\', '\\\\').replace('"', '\\"')
        self.emit(f'    .asciz "{escaped_content}"')
        
        # Switch back to text section
        self.emit(".section .text")
    
    # ============================================================
    # Helper Methods
    # ============================================================
    
    def get_operand(self, operand: str) -> str:
        """
        Get register or immediate for operand
        
        Args:
            operand: Operand name (variable, temp, or constant)
        
        Returns:
            Register name or immediate value
        """
        # Check if constant
        if operand.isdigit() or (operand.startswith('-') and operand[1:].isdigit()):
            # Constant - load into register
            temp_reg = 'r12'  # Use ip as scratch
            value = int(operand)
            if -255 <= value <= 255:
                self.emit(f"    mov {temp_reg}, #{value}")
            else:
                # Large constant - use movw/movt
                self.emit(f"    movw {temp_reg}, #{value & 0xFFFF}")
                if value > 0xFFFF or value < 0:
                    self.emit(f"    movt {temp_reg}, #{(value >> 16) & 0xFFFF}")
            return temp_reg
        
        # Check if string label
        if operand.startswith('str_'):
            # String label - load address into register
            temp_reg = 'r12'  # Use ip as scratch
            self.emit(f"    ldr {temp_reg}, ={operand}")
            return temp_reg
        
        # Variable or temporary - get allocated register
        reg = self.register_allocator.get_register(operand)
        if reg:
            return reg
        
        # Not allocated yet - allocate now
        return self.register_allocator.allocate(operand)
