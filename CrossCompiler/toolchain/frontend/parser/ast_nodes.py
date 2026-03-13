"""
Abstract Syntax Tree Node Definitions
Defines all AST node types for Subset C
"""

from dataclasses import dataclass
from typing import List, Optional


# ============================================================
# Base Classes
# ============================================================

@dataclass
class ASTNode:
    """Base class for all AST nodes"""
    line: int
    column: int


# ============================================================
# Program
# ============================================================

@dataclass
class Program(ASTNode):
    """Root node of AST - represents entire program"""
    declarations: List['Declaration']
    
    def __init__(self, declarations: List['Declaration'], line: int = 1, column: int = 1):
        self.declarations = declarations
        self.line = line
        self.column = column


# ============================================================
# Types
# ============================================================

@dataclass
class Type(ASTNode):
    """
    Type representation
    
    Examples:
        int: base_type="int", pointer_level=0
        char: base_type="char", pointer_level=0
        int*: base_type="int", pointer_level=1
        char**: base_type="char", pointer_level=2
    """
    base_type: str  # "int", "char", or "void"
    pointer_level: int = 0
    
    def __init__(self, base_type: str, pointer_level: int = 0, line: int = 0, column: int = 0):
        self.base_type = base_type
        self.pointer_level = pointer_level
        self.line = line
        self.column = column
    
    def __str__(self) -> str:
        return self.base_type + "*" * self.pointer_level
    
    def is_pointer(self) -> bool:
        return self.pointer_level > 0
    
    def is_void(self) -> bool:
        return self.base_type == "void" and self.pointer_level == 0


# ============================================================
# Declarations
# ============================================================

@dataclass
class Declaration(ASTNode):
    """Base class for declarations"""
    pass


@dataclass
class VarDecl(Declaration):
    """
    Variable declaration
    Examples: int x; int y = 5; char arr[10];
    """
    var_type: Type
    name: str
    array_size: Optional[int] = None  # None for non-arrays
    initializer: Optional['Expression'] = None  # None if no initializer
    
    def __init__(self, var_type: Type, name: str, array_size: Optional[int], line: int, column: int, initializer: Optional['Expression'] = None):
        self.var_type = var_type
        self.name = name
        self.array_size = array_size
        self.initializer = initializer
        self.line = line
        self.column = column
    
    def is_array(self) -> bool:
        return self.array_size is not None


@dataclass
class FunctionDecl(Declaration):
    """
    Function declaration
    Example: int add(int a, int b) { return a + b; }
    """
    return_type: Type
    name: str
    params: List[VarDecl]
    body: 'CompoundStmt'
    
    def __init__(self, return_type: Type, name: str, params: List[VarDecl], body: 'CompoundStmt', line: int, column: int):
        self.return_type = return_type
        self.name = name
        self.params = params
        self.body = body
        self.line = line
        self.column = column


# ============================================================
# Statements
# ============================================================

@dataclass
class Statement(ASTNode):
    """Base class for statements"""
    pass


@dataclass
class CompoundStmt(Statement):
    """
    Compound statement (block)
    Example: { stmt1; stmt2; }
    """
    statements: List[Statement]
    
    def __init__(self, statements: List[Statement], line: int, column: int):
        self.statements = statements
        self.line = line
        self.column = column


@dataclass
class ExprStmt(Statement):
    """
    Expression statement
    Example: x = 5;
    """
    expression: Optional['Expression']  # None for empty statement (;)
    
    def __init__(self, expression: Optional['Expression'], line: int, column: int):
        self.expression = expression
        self.line = line
        self.column = column


@dataclass
class IfStmt(Statement):
    """
    If statement
    Example: if (x > 0) { y = 1; } else { y = 0; }
    """
    condition: 'Expression'
    then_stmt: Statement
    else_stmt: Optional[Statement] = None
    
    def __init__(self, condition: 'Expression', then_stmt: Statement, else_stmt: Optional[Statement], line: int, column: int):
        self.condition = condition
        self.then_stmt = then_stmt
        self.else_stmt = else_stmt
        self.line = line
        self.column = column


@dataclass
class WhileStmt(Statement):
    """
    While loop
    Example: while (x < 10) { x = x + 1; }
    """
    condition: 'Expression'
    body: Statement
    
    def __init__(self, condition: 'Expression', body: Statement, line: int, column: int):
        self.condition = condition
        self.body = body
        self.line = line
        self.column = column


@dataclass
class ForStmt(Statement):
    """
    For loop
    Example: for (i = 0; i < 10; i = i + 1) { sum = sum + i; }
    """
    init: Optional['Expression']
    condition: Optional['Expression']
    increment: Optional['Expression']
    body: Statement
    
    def __init__(self, init: Optional['Expression'], condition: Optional['Expression'], 
                 increment: Optional['Expression'], body: Statement, line: int, column: int):
        self.init = init
        self.condition = condition
        self.increment = increment
        self.body = body
        self.line = line
        self.column = column


@dataclass
class ReturnStmt(Statement):
    """
    Return statement
    Example: return x + 1;
    """
    value: Optional['Expression'] = None  # None for void return
    
    def __init__(self, value: Optional['Expression'], line: int, column: int):
        self.value = value
        self.line = line
        self.column = column


# ============================================================
# Expressions
# ============================================================

@dataclass
class Expression(ASTNode):
    """Base class for expressions"""
    pass


@dataclass
class BinaryOp(Expression):
    """
    Binary operation
    Example: a + b, x == y, p && q
    """
    operator: str  # "+", "-", "*", "/", "%", "==", "!=", "<", ">", etc.
    left: Expression
    right: Expression
    
    def __init__(self, operator: str, left: Expression, right: Expression, line: int, column: int):
        self.operator = operator
        self.left = left
        self.right = right
        self.line = line
        self.column = column


@dataclass
class UnaryOp(Expression):
    """
    Unary operation
    Example: -x, !flag, *ptr, &var
    """
    operator: str  # "-", "!", "*" (deref), "&" (address-of)
    operand: Expression
    
    def __init__(self, operator: str, operand: Expression, line: int, column: int):
        self.operator = operator
        self.operand = operand
        self.line = line
        self.column = column


@dataclass
class Assignment(Expression):
    """
    Assignment expression
    Example: x = 42
    """
    target: Expression  # Must be lvalue (identifier, array access, or dereference)
    value: Expression
    
    def __init__(self, target: Expression, value: Expression, line: int, column: int):
        self.target = target
        self.value = value
        self.line = line
        self.column = column


@dataclass
class FunctionCall(Expression):
    """
    Function call
    Example: add(x, y)
    """
    name: str
    arguments: List[Expression]
    
    def __init__(self, name: str, arguments: List[Expression], line: int, column: int):
        self.name = name
        self.arguments = arguments
        self.line = line
        self.column = column


@dataclass
class ArrayAccess(Expression):
    """
    Array subscript
    Example: arr[i]
    """
    array: Expression
    index: Expression
    
    def __init__(self, array: Expression, index: Expression, line: int, column: int):
        self.array = array
        self.index = index
        self.line = line
        self.column = column


@dataclass
class Identifier(Expression):
    """
    Identifier (variable or function name)
    Example: x, count, factorial
    """
    name: str
    
    def __init__(self, name: str, line: int, column: int):
        self.name = name
        self.line = line
        self.column = column


@dataclass
class IntegerLiteral(Expression):
    """
    Integer literal
    Example: 42, 0, 100
    """
    value: int
    
    def __init__(self, value: int, line: int, column: int):
        self.value = value
        self.line = line
        self.column = column


@dataclass
class CharLiteral(Expression):
    """
    Character literal
    Example: 'A', 'x', '0'
    """
    value: str  # Single character
    
    def __init__(self, value: str, line: int, column: int):
        self.value = value
        self.line = line
        self.column = column


@dataclass
class StringLiteral(Expression):
    """
    String literal
    Example: "Hello World", "test string"
    """
    value: str  # String content
    
    def __init__(self, value: str, line: int, column: int):
        self.value = value
        self.line = line
        self.column = column
