"""
IR Generator Implementation
Converts AST to three-address code intermediate representation
"""

from typing import List, Optional
from ...frontend.parser.ast_nodes import *
from .ir_instructions import *
from .generators import TempGenerator, LabelGenerator


class IRGenerator:
    """
    IR Generator for Subset C
    
    Converts AST to three-address code (TAC) intermediate representation.
    Each instruction has at most 3 operands.
    """
    
    def __init__(self):
        """Initialize IR generator"""
        self.instructions: List[IRInstruction] = []
        self.temp_gen = TempGenerator()
        self.label_gen = LabelGenerator()
        self.string_literals = {}  # Map string content to label
        self.string_counter = 0
    
    def generate(self, program: Program) -> List[IRInstruction]:
        """
        Generate IR for program
        
        Args:
            program: Program AST node
        
        Returns:
            List of IR instructions
        """
        self.instructions = []
        
        for decl in program.declarations:
            self.visit_declaration(decl)
        
        return self.instructions
    
    def emit(self, instruction: IRInstruction) -> None:
        """Add instruction to IR list"""
        self.instructions.append(instruction)
    
    def create_string_literal(self, content: str) -> str:
        """
        Create string literal and return its label
        
        Args:
            content: String content
            
        Returns:
            Label for the string literal
        """
        # Check if string already exists
        if content in self.string_literals:
            return self.string_literals[content]
        
        # Create new string label
        label = f"str_{self.string_counter}"
        self.string_counter += 1
        
        # Store mapping
        self.string_literals[content] = label
        
        # Emit string literal instruction
        self.emit(StringLiteralIR(label, content))
        
        return label
    
    def dump(self) -> str:
        """
        Generate human-readable IR output
        
        Returns:
            String representation of IR
        """
        lines = []
        for instr in self.instructions:
            lines.append(str(instr))
        return "\n".join(lines)
    
    # ============================================================
    # Declaration Visitors
    # ============================================================
    
    def visit_declaration(self, decl: Declaration) -> None:
        """Visit declaration"""
        if isinstance(decl, VarDecl):
            # Global variables - no IR needed (handled by assembler)
            pass
        elif isinstance(decl, FunctionDecl):
            # Skip function prototypes (empty body)
            if decl.body.statements:
                self.visit_function_decl(decl)
    
    def visit_function_decl(self, func_decl: FunctionDecl) -> None:
        """Visit function declaration"""
        # Reset generators for new function
        self.temp_gen.reset()
        self.label_gen.reset()
        
        # Emit function entry
        self.emit(FunctionEntryIR(func_decl.name))
        
        # Generate IR for function body
        self.visit_statement(func_decl.body)
        
        # Emit function exit
        self.emit(FunctionExitIR(func_decl.name))
    
    # ============================================================
    # Statement Visitors
    # ============================================================
    
    def visit_statement(self, stmt: Statement) -> None:
        """Visit statement"""
        if isinstance(stmt, VarDecl):
            self.visit_var_decl_stmt(stmt)
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
    
    def visit_var_decl_stmt(self, var_decl: VarDecl) -> None:
        """Visit variable declaration statement"""
        # If has initializer, generate assignment
        if var_decl.initializer:
            init_temp = self.visit_expression(var_decl.initializer)
            self.emit(AssignIR(var_decl.name, init_temp))
    
    def visit_compound_stmt(self, stmt: CompoundStmt) -> None:
        """Visit compound statement"""
        for s in stmt.statements:
            self.visit_statement(s)
    
    def visit_expr_stmt(self, stmt: ExprStmt) -> None:
        """Visit expression statement"""
        if stmt.expression:
            self.visit_expression(stmt.expression)
    
    def visit_if_stmt(self, stmt: IfStmt) -> None:
        """
        Visit if statement
        
        Generated IR:
            <condition code>
            ifFalse condition goto L_else
            <then code>
            goto L_end
        L_else:
            <else code>
        L_end:
        """
        # Generate condition
        cond_temp = self.visit_expression(stmt.condition)
        
        # Generate labels
        else_label = self.label_gen.new_label()
        end_label = self.label_gen.new_label()
        
        # If condition is false, jump to else
        self.emit(CondGotoIR(cond_temp, else_label, is_true=False))
        
        # Then branch
        self.visit_statement(stmt.then_stmt)
        
        # Jump to end
        self.emit(GotoIR(end_label))
        
        # Else branch
        self.emit(LabelIR(else_label))
        if stmt.else_stmt:
            self.visit_statement(stmt.else_stmt)
        
        # End label
        self.emit(LabelIR(end_label))
    
    def visit_while_stmt(self, stmt: WhileStmt) -> None:
        """
        Visit while statement
        
        Generated IR:
        L_start:
            <condition code>
            ifFalse condition goto L_end
            <body code>
            goto L_start
        L_end:
        """
        # Generate labels
        start_label = self.label_gen.new_label()
        end_label = self.label_gen.new_label()
        
        # Start label
        self.emit(LabelIR(start_label))
        
        # Generate condition
        cond_temp = self.visit_expression(stmt.condition)
        
        # If condition is false, exit loop
        self.emit(CondGotoIR(cond_temp, end_label, is_true=False))
        
        # Body
        self.visit_statement(stmt.body)
        
        # Jump back to start
        self.emit(GotoIR(start_label))
        
        # End label
        self.emit(LabelIR(end_label))
    
    def visit_for_stmt(self, stmt: ForStmt) -> None:
        """
        Visit for statement
        
        Generated IR:
            <init code>
        L_start:
            <condition code>
            ifFalse condition goto L_end
            <body code>
            <increment code>
            goto L_start
        L_end:
        """
        # Init
        if stmt.init:
            self.visit_expression(stmt.init)
        
        # Generate labels
        start_label = self.label_gen.new_label()
        end_label = self.label_gen.new_label()
        
        # Start label
        self.emit(LabelIR(start_label))
        
        # Condition
        if stmt.condition:
            cond_temp = self.visit_expression(stmt.condition)
            self.emit(CondGotoIR(cond_temp, end_label, is_true=False))
        
        # Body
        self.visit_statement(stmt.body)
        
        # Increment
        if stmt.increment:
            self.visit_expression(stmt.increment)
        
        # Jump back to start
        self.emit(GotoIR(start_label))
        
        # End label
        self.emit(LabelIR(end_label))
    
    def visit_return_stmt(self, stmt: ReturnStmt) -> None:
        """Visit return statement"""
        if stmt.value:
            value_temp = self.visit_expression(stmt.value)
            self.emit(ReturnIR(value_temp))
        else:
            self.emit(ReturnIR())
    
    # ============================================================
    # Expression Visitors
    # ============================================================
    
    def visit_expression(self, expr: Expression) -> str:
        """
        Visit expression and return temporary holding result
        
        Args:
            expr: Expression node
        
        Returns:
            Temporary variable name or constant
        """
        if isinstance(expr, Identifier):
            return expr.name
        
        elif isinstance(expr, IntegerLiteral):
            return str(expr.value)
        
        elif isinstance(expr, CharLiteral):
            # Convert char to ASCII value
            return str(ord(expr.value))
        
        elif isinstance(expr, StringLiteral):
            # Create string literal in data section
            label = self.create_string_literal(expr.value)
            return label
        
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
        
        return "0"
    
    def visit_binary_op(self, expr: BinaryOp) -> str:
        """Visit binary operation"""
        left_temp = self.visit_expression(expr.left)
        right_temp = self.visit_expression(expr.right)
        
        result_temp = self.temp_gen.new_temp()
        self.emit(BinaryOpIR(result_temp, expr.operator, left_temp, right_temp))
        
        return result_temp
    
    def visit_unary_op(self, expr: UnaryOp) -> str:
        """Visit unary operation"""
        operand_temp = self.visit_expression(expr.operand)
        
        # Special handling for address-of and dereference
        if expr.operator == '&':
            # Address-of: result = &operand
            result_temp = self.temp_gen.new_temp()
            self.emit(UnaryOpIR(result_temp, '&', operand_temp))
            return result_temp
        
        elif expr.operator == '*':
            # Dereference: result = *operand
            result_temp = self.temp_gen.new_temp()
            self.emit(LoadIR(result_temp, operand_temp))
            return result_temp
        
        else:
            # Regular unary op: -, !
            result_temp = self.temp_gen.new_temp()
            self.emit(UnaryOpIR(result_temp, expr.operator, operand_temp))
            return result_temp
    
    def visit_assignment(self, expr: Assignment) -> str:
        """Visit assignment"""
        value_temp = self.visit_expression(expr.value)
        
        # Check if target is simple identifier or needs memory store
        if isinstance(expr.target, Identifier):
            # Simple assignment: x = value
            self.emit(AssignIR(expr.target.name, value_temp))
            return expr.target.name
        
        elif isinstance(expr.target, ArrayAccess):
            # Array assignment: arr[i] = value
            # First compute array address
            array_temp = self.visit_expression(expr.target.array)
            index_temp = self.visit_expression(expr.target.index)
            
            # Compute address: addr = array + index
            addr_temp = self.temp_gen.new_temp()
            self.emit(BinaryOpIR(addr_temp, '+', array_temp, index_temp))
            
            # Store: *addr = value
            self.emit(StoreIR(addr_temp, value_temp))
            return value_temp
        
        elif isinstance(expr.target, UnaryOp) and expr.target.operator == '*':
            # Pointer dereference assignment: *ptr = value
            ptr_temp = self.visit_expression(expr.target.operand)
            self.emit(StoreIR(ptr_temp, value_temp))
            return value_temp
        
        else:
            # Should not reach here (semantic analyzer should catch this)
            return value_temp
    
    def visit_function_call(self, expr: FunctionCall) -> str:
        """Visit function call"""
        # Generate param instructions for arguments
        for arg in expr.arguments:
            arg_temp = self.visit_expression(arg)
            self.emit(ParamIR(arg_temp))
        
        # Generate call instruction
        result_temp = self.temp_gen.new_temp()
        self.emit(CallIR(result_temp, expr.name, len(expr.arguments)))
        
        return result_temp
    
    def visit_array_access(self, expr: ArrayAccess) -> str:
        """Visit array access"""
        # Compute array address
        array_temp = self.visit_expression(expr.array)
        index_temp = self.visit_expression(expr.index)
        
        # Compute address: addr = array + index
        addr_temp = self.temp_gen.new_temp()
        self.emit(BinaryOpIR(addr_temp, '+', array_temp, index_temp))
        
        # Load: result = *addr
        result_temp = self.temp_gen.new_temp()
        self.emit(LoadIR(result_temp, addr_temp))
        
        return result_temp
