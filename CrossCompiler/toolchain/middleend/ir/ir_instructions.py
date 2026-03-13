"""
IR Instruction Definitions
Three-address code intermediate representation
"""

from dataclasses import dataclass
from typing import Optional


@dataclass
class IRInstruction:
    """Base class for IR instructions"""
    
    def __str__(self) -> str:
        """String representation for debugging"""
        return self.__class__.__name__


# ============================================================
# Arithmetic and Logic Instructions
# ============================================================

@dataclass
class BinaryOpIR(IRInstruction):
    """
    Binary operation: result = left op right
    Examples: t0 = a + b, t1 = x * y
    """
    result: str
    operator: str
    left: str
    right: str
    
    def __str__(self) -> str:
        return f"{self.result} = {self.left} {self.operator} {self.right}"


@dataclass
class UnaryOpIR(IRInstruction):
    """
    Unary operation: result = op operand
    Examples: t0 = -x, t1 = !flag
    """
    result: str
    operator: str
    operand: str
    
    def __str__(self) -> str:
        return f"{self.result} = {self.operator}{self.operand}"


@dataclass
class AssignIR(IRInstruction):
    """
    Assignment: dest = source
    Examples: x = t0, t1 = 42
    """
    dest: str
    source: str
    
    def __str__(self) -> str:
        return f"{self.dest} = {self.source}"


# ============================================================
# Memory Instructions
# ============================================================

@dataclass
class LoadIR(IRInstruction):
    """
    Load from memory: result = *address
    Examples: t0 = *ptr, t1 = arr[i]
    """
    result: str
    address: str
    
    def __str__(self) -> str:
        return f"{self.result} = *{self.address}"


@dataclass
class StoreIR(IRInstruction):
    """
    Store to memory: *address = value
    Examples: *ptr = x, arr[i] = 5
    """
    address: str
    value: str
    
    def __str__(self) -> str:
        return f"*{self.address} = {self.value}"


# ============================================================
# Control Flow Instructions
# ============================================================

@dataclass
class LabelIR(IRInstruction):
    """
    Label: L0:
    """
    label: str
    
    def __str__(self) -> str:
        return f"{self.label}:"


@dataclass
class GotoIR(IRInstruction):
    """
    Unconditional jump: goto L0
    """
    label: str
    
    def __str__(self) -> str:
        return f"goto {self.label}"


@dataclass
class CondGotoIR(IRInstruction):
    """
    Conditional jump: if condition goto label
    Examples: if t0 goto L1, ifFalse t1 goto L2
    """
    condition: str
    label: str
    is_true: bool = True  # True for "if", False for "ifFalse"
    
    def __str__(self) -> str:
        prefix = "if" if self.is_true else "ifFalse"
        return f"{prefix} {self.condition} goto {self.label}"


# ============================================================
# Function Instructions
# ============================================================

@dataclass
class FunctionEntryIR(IRInstruction):
    """
    Function entry: function name:
    """
    name: str
    
    def __str__(self) -> str:
        return f"function {self.name}:"


@dataclass
class FunctionExitIR(IRInstruction):
    """
    Function exit: end function
    """
    name: str
    
    def __str__(self) -> str:
        return f"end function {self.name}"


@dataclass
class ParamIR(IRInstruction):
    """
    Function parameter: param value
    Used before CallIR to pass arguments
    """
    value: str
    
    def __str__(self) -> str:
        return f"param {self.value}"


@dataclass
class CallIR(IRInstruction):
    """
    Function call: result = call function, num_args
    Examples: t0 = call add, 2
    """
    result: Optional[str]  # None for void functions
    function: str
    num_args: int
    
    def __str__(self) -> str:
        if self.result:
            return f"{self.result} = call {self.function}, {self.num_args}"
        else:
            return f"call {self.function}, {self.num_args}"


@dataclass
class ReturnIR(IRInstruction):
    """
    Return from function: return value
    Examples: return t0, return (for void)
    """
    value: Optional[str] = None  # None for void return
    
    def __str__(self) -> str:
        if self.value:
            return f"return {self.value}"
        else:
            return "return"


# ============================================================
# Data Instructions
# ============================================================

@dataclass
class StringLiteralIR(IRInstruction):
    """
    String literal: string label "content"
    Examples: string str_0 "Hello World"
    """
    label: str
    content: str
    
    def __str__(self) -> str:
        # Escape special characters for display
        escaped = self.content.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
        return f'string {self.label} "{escaped}"'
