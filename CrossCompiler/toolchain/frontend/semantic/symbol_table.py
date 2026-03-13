"""
Symbol Table Implementation
Manages symbols and scopes for semantic analysis
"""

from dataclasses import dataclass
from typing import Optional, Dict, List
from ..parser.ast_nodes import Type


@dataclass
class Symbol:
    """
    Symbol table entry for variables
    
    Attributes:
        name: Symbol name
        symbol_type: "variable" or "parameter"
        var_type: Type information (Type object)
        scope_level: Nesting level (0 = global, 1+ = local)
        offset: Stack offset for local variables (used in code generation)
        array_size: Array size if array, None otherwise
    """
    name: str
    symbol_type: str  # "variable" or "parameter"
    var_type: Type
    scope_level: int
    offset: int = 0
    array_size: Optional[int] = None
    
    def is_array(self) -> bool:
        return self.array_size is not None


@dataclass
class FunctionSymbol:
    """
    Symbol table entry for functions
    
    Attributes:
        name: Function name
        return_type: Return type (Type object)
        param_types: List of parameter types
        scope_level: Always 0 (functions are global)
    """
    name: str
    return_type: Type
    param_types: List[Type]
    scope_level: int = 0


class SymbolTable:
    """
    Symbol table with scope management
    
    Manages nested scopes using a stack of dictionaries.
    Each scope is a dictionary mapping symbol names to Symbol/FunctionSymbol objects.
    """
    
    def __init__(self):
        """Initialize symbol table with global scope"""
        self.scopes: List[Dict[str, Symbol | FunctionSymbol]] = [{}]  # Stack of scopes
        self.current_scope_level = 0
    
    def enter_scope(self) -> None:
        """Enter a new scope (push new scope onto stack)"""
        self.scopes.append({})
        self.current_scope_level += 1
    
    def exit_scope(self) -> None:
        """Exit current scope (pop scope from stack)"""
        if len(self.scopes) > 1:
            self.scopes.pop()
            self.current_scope_level -= 1
    
    def declare(self, name: str, symbol: Symbol | FunctionSymbol) -> bool:
        """
        Declare a symbol in current scope
        
        Args:
            name: Symbol name
            symbol: Symbol or FunctionSymbol object
        
        Returns:
            True if declaration successful, False if duplicate in current scope
        """
        current_scope = self.scopes[-1]
        
        # Check for duplicate in current scope
        if name in current_scope:
            return False
        
        current_scope[name] = symbol
        return True
    
    def lookup(self, name: str) -> Optional[Symbol | FunctionSymbol]:
        """
        Look up symbol in current and enclosing scopes
        
        Args:
            name: Symbol name to look up
        
        Returns:
            Symbol/FunctionSymbol if found, None otherwise
        """
        # Search from innermost to outermost scope
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        
        return None
    
    def lookup_current_scope(self, name: str) -> Optional[Symbol | FunctionSymbol]:
        """
        Look up symbol in current scope only
        
        Args:
            name: Symbol name to look up
        
        Returns:
            Symbol/FunctionSymbol if found in current scope, None otherwise
        """
        current_scope = self.scopes[-1]
        return current_scope.get(name)
    
    def get_scope_level(self) -> int:
        """Get current scope nesting level"""
        return self.current_scope_level
