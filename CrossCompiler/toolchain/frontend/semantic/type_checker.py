"""
Type Checker Implementation
Handles type compatibility checking and type inference
"""

from typing import Optional
from ..parser.ast_nodes import Type, Expression, Identifier, ArrayAccess, UnaryOp


class TypeChecker:
    """
    Type checker for Subset C
    
    Handles:
    - Type compatibility checking
    - Result type inference for operations
    - Lvalue validation
    """
    
    def are_compatible(self, type1: Type, type2: Type) -> bool:
        """
        Check if two types are compatible for assignment
        
        Rules:
        - int = int (compatible)
        - char = char (compatible)
        - int* = int* (compatible, same pointer level)
        - char = int (compatible, implicit narrowing)
        - int = char (compatible, implicit widening)
        - Pointer types must match exactly (base type and pointer level)
        
        Args:
            type1: Target type (left side of assignment)
            type2: Source type (right side of assignment)
        
        Returns:
            True if compatible, False otherwise
        """
        # Exact match
        if type1.base_type == type2.base_type and type1.pointer_level == type2.pointer_level:
            return True
        
        # Pointer types must match exactly
        if type1.is_pointer() or type2.is_pointer():
            return False
        
        # int and char are compatible (implicit conversion)
        if type1.base_type in ("int", "char") and type2.base_type in ("int", "char"):
            return True
        
        return False
    
    def get_result_type(self, operator: str, left_type: Type, right_type: Optional[Type] = None) -> Optional[Type]:
        """
        Get result type of an operation
        
        Args:
            operator: Operator string (+, -, *, /, etc.)
            left_type: Type of left operand
            right_type: Type of right operand (None for unary operators)
        
        Returns:
            Result type, or None if operation is invalid
        """
        # Unary operators
        if right_type is None:
            if operator == '-':
                # Negation: int/char → int
                if left_type.base_type in ("int", "char") and not left_type.is_pointer():
                    return Type("int", 0, 0, 0)
            
            elif operator == '!':
                # Logical NOT: any → int
                return Type("int", 0, 0, 0)
            
            elif operator == '*':
                # Dereference: T* → T
                if left_type.is_pointer():
                    return Type(left_type.base_type, left_type.pointer_level - 1, 0, 0)
            
            elif operator == '&':
                # Address-of: T → T*
                return Type(left_type.base_type, left_type.pointer_level + 1, 0, 0)
            
            return None
        
        # Binary operators
        # Arithmetic operators: +, -, *, /, %
        if operator in ('+', '-', '*', '/', '%'):
            # Pointer arithmetic
            if operator in ('+', '-'):
                # ptr + int or int + ptr → ptr
                if left_type.is_pointer() and right_type.base_type in ("int", "char") and not right_type.is_pointer():
                    return left_type
                if right_type.is_pointer() and left_type.base_type in ("int", "char") and not left_type.is_pointer():
                    return right_type
                # ptr - ptr → int
                if operator == '-' and left_type.is_pointer() and right_type.is_pointer():
                    if left_type.base_type == right_type.base_type and left_type.pointer_level == right_type.pointer_level:
                        return Type("int", 0, 0, 0)
            
            # Regular arithmetic: int/char op int/char → int
            if left_type.base_type in ("int", "char") and not left_type.is_pointer():
                if right_type.base_type in ("int", "char") and not right_type.is_pointer():
                    return Type("int", 0, 0, 0)
        
        # Comparison operators: ==, !=, <, >, <=, >=
        elif operator in ('==', '!=', '<', '>', '<=', '>='):
            # Can compare int/char or pointers of same type
            if not left_type.is_pointer() and not right_type.is_pointer():
                if left_type.base_type in ("int", "char") and right_type.base_type in ("int", "char"):
                    return Type("int", 0, 0, 0)
            elif left_type.is_pointer() and right_type.is_pointer():
                if left_type.base_type == right_type.base_type and left_type.pointer_level == right_type.pointer_level:
                    return Type("int", 0, 0, 0)
        
        # Logical operators: &&, ||
        elif operator in ('&&', '||'):
            # Any type can be used in logical context → int result
            return Type("int", 0, 0, 0)
        
        # Bitwise operators: &, |, ^, <<, >>
        elif operator in ('&', '|', '^', '<<', '>>'):
            # Only int/char operands, no pointers
            if not left_type.is_pointer() and not right_type.is_pointer():
                if left_type.base_type in ("int", "char") and right_type.base_type in ("int", "char"):
                    return Type("int", 0, 0, 0)
        
        return None
    
    def is_lvalue(self, expr: Expression) -> bool:
        """
        Check if expression is an lvalue (can be assigned to)
        
        Lvalues:
        - Identifiers (variables)
        - Array access (arr[i])
        - Pointer dereference (*ptr)
        
        Args:
            expr: Expression to check
        
        Returns:
            True if lvalue, False otherwise
        """
        if isinstance(expr, Identifier):
            return True
        
        if isinstance(expr, ArrayAccess):
            return True
        
        if isinstance(expr, UnaryOp) and expr.operator == '*':
            return True
        
        return False
