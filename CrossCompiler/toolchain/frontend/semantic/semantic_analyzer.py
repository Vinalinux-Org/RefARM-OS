"""
Semantic Analyzer Implementation
Performs semantic analysis on AST: symbol resolution, type checking, scope validation
"""

from typing import Optional
from ..parser.ast_nodes import *
from .symbol_table import SymbolTable, Symbol, FunctionSymbol
from .type_checker import TypeChecker
from ...common.error import ErrorCollector


class SemanticAnalyzer:
    """
    Semantic analyzer for Subset C
    
    Performs:
    - Symbol table construction
    - Scope management
    - Declaration checking (undeclared variables/functions)
    - Type checking
    - Duplicate declaration detection
    """
    
    def __init__(self, filename: str, error_collector: ErrorCollector):
        """
        Initialize semantic analyzer
        
        Args:
            filename: Source file name (for error reporting)
            error_collector: Error collector for reporting semantic errors
        """
        self.filename = filename
        self.error_collector = error_collector
        self.symbol_table = SymbolTable()
        self.type_checker = TypeChecker()
        self.current_function_return_type: Optional[Type] = None
    
    def analyze(self, program: Program) -> bool:
        """
        Analyze program AST
        
        Args:
            program: Program AST node
        
        Returns:
            True if analysis successful, False if errors found
        """
        # First pass: collect all function declarations
        for decl in program.declarations:
            if isinstance(decl, FunctionDecl):
                self.declare_function(decl)
        
        # Second pass: analyze all declarations
        for decl in program.declarations:
            self.visit_declaration(decl)
        
        return not self.error_collector.has_errors()
    
    def declare_function(self, func_decl: FunctionDecl) -> None:
        """Declare function in symbol table (first pass)"""
        param_types = [param.var_type for param in func_decl.params]
        func_symbol = FunctionSymbol(
            name=func_decl.name,
            return_type=func_decl.return_type,
            param_types=param_types,
            scope_level=0
        )
        
        if not self.symbol_table.declare(func_decl.name, func_symbol):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                func_decl.line,
                func_decl.column,
                f"Duplicate function declaration: {func_decl.name}"
            )
    
    # ============================================================
    # Declaration Visitors
    # ============================================================
    
    def visit_declaration(self, decl: Declaration) -> None:
        """Visit declaration node"""
        if isinstance(decl, VarDecl):
            self.visit_var_decl(decl)
        elif isinstance(decl, FunctionDecl):
            self.visit_function_decl(decl)
    
    def visit_var_decl(self, var_decl: VarDecl) -> None:
        """Visit variable declaration"""
        # For arrays, the variable has pointer type
        # Example: int arr[10] → arr has type int*
        var_type = var_decl.var_type
        if var_decl.array_size is not None:
            # Array: increase pointer level by 1
            var_type = Type(var_type.base_type, var_type.pointer_level + 1, var_type.line, var_type.column)
        
        # Create symbol
        symbol = Symbol(
            name=var_decl.name,
            symbol_type="variable",
            var_type=var_type,
            scope_level=self.symbol_table.get_scope_level(),
            array_size=var_decl.array_size
        )
        
        # Declare in symbol table
        if not self.symbol_table.declare(var_decl.name, symbol):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                var_decl.line,
                var_decl.column,
                f"Duplicate variable declaration: {var_decl.name}"
            )
        
        # Check initializer if present
        if var_decl.initializer:
            init_type = self.visit_expression(var_decl.initializer)
            if init_type and not self.type_checker.are_compatible(var_decl.var_type, init_type):
                self.error_collector.add_error(
                    "semantic",
                    self.filename,
                    var_decl.line,
                    var_decl.column,
                    f"Type mismatch in initializer: cannot assign {init_type} to {var_decl.var_type}"
                )
    
    def visit_function_decl(self, func_decl: FunctionDecl) -> None:
        """Visit function declaration"""
        # Skip function prototypes (empty body)
        if not func_decl.body.statements:
            return
        
        # Function already declared in first pass
        # Enter function scope
        self.symbol_table.enter_scope()
        self.current_function_return_type = func_decl.return_type
        
        # Declare parameters
        for param in func_decl.params:
            # For array parameters, treat as pointer
            param_type = param.var_type
            if param.array_size is not None:
                param_type = Type(param_type.base_type, param_type.pointer_level + 1, param_type.line, param_type.column)
            
            param_symbol = Symbol(
                name=param.name,
                symbol_type="parameter",
                var_type=param_type,
                scope_level=self.symbol_table.get_scope_level(),
                array_size=param.array_size
            )
            
            if not self.symbol_table.declare(param.name, param_symbol):
                self.error_collector.add_error(
                    "semantic",
                    self.filename,
                    param.line,
                    param.column,
                    f"Duplicate parameter name: {param.name}"
                )
        
        # Analyze function body
        self.visit_statement(func_decl.body)
        
        # Exit function scope
        self.symbol_table.exit_scope()
        self.current_function_return_type = None
    
    # ============================================================
    # Statement Visitors
    # ============================================================
    
    def visit_statement(self, stmt: Statement) -> None:
        """Visit statement node"""
        if isinstance(stmt, VarDecl):
            self.visit_var_decl(stmt)
        elif isinstance(stmt, CompoundStmt):
            self.visit_compound_stmt(stmt)
        elif isinstance(stmt, ExprStmt):
            self.visit_expr_stmt(stmt)
        elif isinstance(stmt, IfStmt):
            self.visit_if_stmt(stmt)
        elif isinstance(stmt, WhileStmt):
            self.visit_while_stmt(stmt)
        elif isinstance(stmt, ForStmt):
            self.visit_for_stmt(stmt)
        elif isinstance(stmt, ReturnStmt):
            self.visit_return_stmt(stmt)
    
    def visit_compound_stmt(self, stmt: CompoundStmt) -> None:
        """Visit compound statement"""
        # Enter new scope
        self.symbol_table.enter_scope()
        
        for s in stmt.statements:
            self.visit_statement(s)
        
        # Exit scope
        self.symbol_table.exit_scope()
    
    def visit_expr_stmt(self, stmt: ExprStmt) -> None:
        """Visit expression statement"""
        if stmt.expression:
            self.visit_expression(stmt.expression)
    
    def visit_if_stmt(self, stmt: IfStmt) -> None:
        """Visit if statement"""
        # Check condition
        self.visit_expression(stmt.condition)
        
        # Check then branch
        self.visit_statement(stmt.then_stmt)
        
        # Check else branch if present
        if stmt.else_stmt:
            self.visit_statement(stmt.else_stmt)
    
    def visit_while_stmt(self, stmt: WhileStmt) -> None:
        """Visit while statement"""
        # Check condition
        self.visit_expression(stmt.condition)
        
        # Check body
        self.visit_statement(stmt.body)
    
    def visit_for_stmt(self, stmt: ForStmt) -> None:
        """Visit for statement"""
        # Check init
        if stmt.init:
            self.visit_expression(stmt.init)
        
        # Check condition
        if stmt.condition:
            self.visit_expression(stmt.condition)
        
        # Check increment
        if stmt.increment:
            self.visit_expression(stmt.increment)
        
        # Check body
        self.visit_statement(stmt.body)
    
    def visit_return_stmt(self, stmt: ReturnStmt) -> None:
        """Visit return statement"""
        if stmt.value:
            return_type = self.visit_expression(stmt.value)
            
            # Check return type matches function return type
            if self.current_function_return_type and return_type:
                if not self.type_checker.are_compatible(self.current_function_return_type, return_type):
                    self.error_collector.add_error(
                        "semantic",
                        self.filename,
                        stmt.line,
                        stmt.column,
                        f"Return type mismatch: expected {self.current_function_return_type}, got {return_type}"
                    )
        else:
            # Empty return - check if function is void
            if self.current_function_return_type and not self.current_function_return_type.is_void():
                self.error_collector.add_error(
                    "semantic",
                    self.filename,
                    stmt.line,
                    stmt.column,
                    f"Return statement missing value in non-void function"
                )
    
    # ============================================================
    # Expression Visitors
    # ============================================================
    
    def visit_expression(self, expr: Expression) -> Optional[Type]:
        """
        Visit expression and return its type
        
        Args:
            expr: Expression node
        
        Returns:
            Type of expression, or None if type cannot be determined
        """
        if isinstance(expr, Identifier):
            return self.visit_identifier(expr)
        elif isinstance(expr, IntegerLiteral):
            return Type("int", 0, expr.line, expr.column)
        elif isinstance(expr, CharLiteral):
            return Type("char", 0, expr.line, expr.column)
        elif isinstance(expr, StringLiteral):
            return Type("char", 1, expr.line, expr.column)  # char* (pointer to char)
        elif isinstance(expr, BinaryOp):
            return self.visit_binary_op(expr)
        elif isinstance(expr, UnaryOp):
            return self.visit_unary_op(expr)
        elif isinstance(expr, Assignment):
            return self.visit_assignment(expr)
        elif isinstance(expr, FunctionCall):
            return self.visit_function_call(expr)
        elif isinstance(expr, ArrayAccess):
            return self.visit_array_access(expr)
        
        return None
    
    def visit_identifier(self, expr: Identifier) -> Optional[Type]:
        """Visit identifier"""
        symbol = self.symbol_table.lookup(expr.name)
        
        if not symbol:
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Undeclared variable: {expr.name}"
            )
            return None
        
        if isinstance(symbol, FunctionSymbol):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Function name used as variable: {expr.name}"
            )
            return None
        
        return symbol.var_type
    
    def visit_binary_op(self, expr: BinaryOp) -> Optional[Type]:
        """Visit binary operation"""
        left_type = self.visit_expression(expr.left)
        right_type = self.visit_expression(expr.right)
        
        if not left_type or not right_type:
            return None
        
        result_type = self.type_checker.get_result_type(expr.operator, left_type, right_type)
        
        if not result_type:
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Invalid operand types for operator {expr.operator}: {left_type} and {right_type}"
            )
            return None
        
        return result_type
    
    def visit_unary_op(self, expr: UnaryOp) -> Optional[Type]:
        """Visit unary operation"""
        operand_type = self.visit_expression(expr.operand)
        
        if not operand_type:
            return None
        
        result_type = self.type_checker.get_result_type(expr.operator, operand_type)
        
        if not result_type:
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Invalid operand type for operator {expr.operator}: {operand_type}"
            )
            return None
        
        # For address-of operator, check if operand is lvalue
        if expr.operator == '&':
            if not self.type_checker.is_lvalue(expr.operand):
                self.error_collector.add_error(
                    "semantic",
                    self.filename,
                    expr.line,
                    expr.column,
                    "Address-of operator requires lvalue"
                )
        
        return result_type
    
    def visit_assignment(self, expr: Assignment) -> Optional[Type]:
        """Visit assignment"""
        # Check if target is lvalue
        if not self.type_checker.is_lvalue(expr.target):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                "Assignment target must be lvalue"
            )
        
        target_type = self.visit_expression(expr.target)
        value_type = self.visit_expression(expr.value)
        
        if not target_type or not value_type:
            return None
        
        # Check type compatibility
        if not self.type_checker.are_compatible(target_type, value_type):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Type mismatch in assignment: cannot assign {value_type} to {target_type}"
            )
        
        return target_type
    
    def visit_function_call(self, expr: FunctionCall) -> Optional[Type]:
        """Visit function call"""
        # Look up function
        symbol = self.symbol_table.lookup(expr.name)
        
        if not symbol:
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Undeclared function: {expr.name}"
            )
            return None
        
        if not isinstance(symbol, FunctionSymbol):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Variable used as function: {expr.name}"
            )
            return None
        
        # Check argument count
        if len(expr.arguments) != len(symbol.param_types):
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Function {expr.name} expects {len(symbol.param_types)} arguments, got {len(expr.arguments)}"
            )
            return symbol.return_type
        
        # Check argument types
        for i, (arg, param_type) in enumerate(zip(expr.arguments, symbol.param_types)):
            arg_type = self.visit_expression(arg)
            if arg_type and not self.type_checker.are_compatible(param_type, arg_type):
                self.error_collector.add_error(
                    "semantic",
                    self.filename,
                    arg.line,
                    arg.column,
                    f"Argument {i+1} type mismatch: expected {param_type}, got {arg_type}"
                )
        
        return symbol.return_type
    
    def visit_array_access(self, expr: ArrayAccess) -> Optional[Type]:
        """Visit array access"""
        array_type = self.visit_expression(expr.array)
        index_type = self.visit_expression(expr.index)
        
        if not array_type or not index_type:
            return None
        
        # Check index is integer type
        if index_type.base_type not in ("int", "char") or index_type.is_pointer():
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Array index must be integer type, got {index_type}"
            )
        
        # Array access on pointer or array
        # For arrays: int arr[10] → arr has type int*, arr[i] has type int
        # For pointers: int* ptr → ptr[i] has type int
        if array_type.is_pointer():
            # Dereference pointer
            return Type(array_type.base_type, array_type.pointer_level - 1, expr.line, expr.column)
        else:
            self.error_collector.add_error(
                "semantic",
                self.filename,
                expr.line,
                expr.column,
                f"Array subscript requires pointer or array type, got {array_type}"
            )
            return None
