"""
Recursive Descent Parser
Parses token stream into Abstract Syntax Tree
"""

from typing import List, Optional
from ..lexer.token import Token, TokenType
from .ast_nodes import *
from ...common.error import ErrorCollector


class Parser:
    """
    Recursive descent parser for Subset C
    Implements operator precedence climbing for expressions
    """
    
    def __init__(self, tokens: List[Token], filename: str, error_collector: ErrorCollector):
        """
        Initialize parser
        
        Args:
            tokens: List of tokens from lexer
            filename: Source file name (for error reporting)
            error_collector: Error collector for reporting syntax errors
        """
        self.tokens = tokens
        self.filename = filename
        self.error_collector = error_collector
        
        self.pos = 0
        self.current_token = self.tokens[0] if tokens else None
    
    def advance(self) -> None:
        """Move to next token"""
        self.pos += 1
        if self.pos < len(self.tokens):
            self.current_token = self.tokens[self.pos]
        else:
            self.current_token = None
    
    def peek(self, offset: int = 1) -> Optional[Token]:
        """Look ahead at token without advancing"""
        peek_pos = self.pos + offset
        if peek_pos < len(self.tokens):
            return self.tokens[peek_pos]
        return None
    
    def expect(self, token_type: TokenType) -> Token:
        """
        Expect current token to be of given type, consume it
        Report error if mismatch
        """
        if self.current_token and self.current_token.type == token_type:
            token = self.current_token
            self.advance()
            return token
        else:
            # Syntax error
            current_type = self.current_token.type.name if self.current_token else "EOF"
            self.error_collector.add_error(
                "parser",
                self.filename,
                self.current_token.line if self.current_token else 0,
                self.current_token.column if self.current_token else 0,
                f"Expected {token_type.name}, got {current_type}"
            )
            # Return dummy token to continue parsing
            return Token(token_type, "", 
                        self.current_token.line if self.current_token else 0,
                        self.current_token.column if self.current_token else 0)
    
    def match(self, *token_types: TokenType) -> bool:
        """Check if current token matches any of given types"""
        if not self.current_token:
            return False
        return self.current_token.type in token_types
    
    def synchronize(self) -> None:
        """
        Error recovery: skip tokens until synchronization point
        Synchronization points: semicolon, closing brace, EOF
        """
        while self.current_token and self.current_token.type != TokenType.EOF:
            if self.current_token.type in (TokenType.SEMICOLON, TokenType.RBRACE):
                self.advance()
                return
            self.advance()
    
    # ============================================================
    # Parser Entry Point
    # ============================================================
    
    def parse(self) -> Optional[Program]:
        """
        Parse token stream into AST
        Returns Program node or None if parsing fails
        """
        declarations = []
        
        while self.current_token and self.current_token.type != TokenType.EOF:
            try:
                decl = self.parse_declaration()
                if decl:
                    declarations.append(decl)
            except Exception as e:
                # Error already reported, synchronize and continue
                self.synchronize()
        
        if self.error_collector.has_errors():
            return None
        
        return Program(declarations, 1, 1)
    
    # ============================================================
    # Declarations
    # ============================================================
    
    def parse_declaration(self) -> Optional[Declaration]:
        """
        Parse declaration (function or variable)
        
        Grammar:
            declaration → type IDENTIFIER ( '(' param_list? ')' compound_stmt | ('[' INTEGER ']')? ';' )
        """
        # Parse type
        var_type = self.parse_type()
        if not var_type:
            return None
        
        # Parse identifier
        if not self.match(TokenType.IDENTIFIER):
            self.error_collector.add_error(
                "parser",
                self.filename,
                self.current_token.line if self.current_token else 0,
                self.current_token.column if self.current_token else 0,
                "Expected identifier in declaration"
            )
            self.synchronize()
            return None
        
        name_token = self.current_token
        name = name_token.value
        self.advance()
        
        # Check if function or variable
        if self.match(TokenType.LPAREN):
            # Function declaration
            return self.parse_function_decl(var_type, name, name_token.line, name_token.column)
        else:
            # Variable declaration
            return self.parse_var_decl(var_type, name, name_token.line, name_token.column)
    
    def parse_type(self) -> Optional[Type]:
        """
        Parse type (int, char, void, with optional pointer levels)
        
        Grammar:
            type → ('int' | 'char' | 'void') '*'*
        """
        if not self.match(TokenType.INT, TokenType.CHAR, TokenType.VOID):
            return None
        
        base_type_token = self.current_token
        base_type = base_type_token.value
        line = base_type_token.line
        column = base_type_token.column
        self.advance()
        
        # Count pointer levels
        pointer_level = 0
        while self.match(TokenType.STAR):
            pointer_level += 1
            self.advance()
        
        return Type(base_type, pointer_level, line, column)
    
    def parse_function_decl(self, return_type: Type, name: str, line: int, column: int) -> FunctionDecl:
        """Parse function declaration"""
        self.expect(TokenType.LPAREN)
        
        # Parse parameters
        params = []
        if not self.match(TokenType.RPAREN):
            params = self.parse_param_list()
        
        self.expect(TokenType.RPAREN)
        
        # Check if function prototype (no body) or definition (with body)
        if self.match(TokenType.SEMICOLON):
            # Function prototype - create empty body
            self.advance()
            body = CompoundStmt([], line, column)
            return FunctionDecl(return_type, name, params, body, line, column)
        
        # Parse body
        body = self.parse_compound_stmt()
        
        return FunctionDecl(return_type, name, params, body, line, column)
    
    def parse_param_list(self) -> List[VarDecl]:
        """Parse function parameter list"""
        params = []
        
        # First parameter
        param_type = self.parse_type()
        if not param_type:
            return params
        
        if not self.match(TokenType.IDENTIFIER):
            self.error_collector.add_error(
                "parser",
                self.filename,
                self.current_token.line if self.current_token else 0,
                self.current_token.column if self.current_token else 0,
                "Expected parameter name"
            )
            return params
        
        param_name = self.current_token.value
        param_line = self.current_token.line
        param_column = self.current_token.column
        self.advance()
        
        params.append(VarDecl(param_type, param_name, None, param_line, param_column, None))
        
        # Additional parameters
        while self.match(TokenType.COMMA):
            self.advance()
            
            param_type = self.parse_type()
            if not param_type:
                break
            
            if not self.match(TokenType.IDENTIFIER):
                self.error_collector.add_error(
                    "parser",
                    self.filename,
                    self.current_token.line if self.current_token else 0,
                    self.current_token.column if self.current_token else 0,
                    "Expected parameter name"
                )
                break
            
            param_name = self.current_token.value
            param_line = self.current_token.line
            param_column = self.current_token.column
            self.advance()
            
            params.append(VarDecl(param_type, param_name, None, param_line, param_column, None))
        
        return params
    
    def parse_var_decl(self, var_type: Type, name: str, line: int, column: int) -> VarDecl:
        """Parse variable declaration with optional initializer"""
        array_size = None
        initializer = None
        
        # Check for array
        if self.match(TokenType.LBRACKET):
            self.advance()
            
            if not self.match(TokenType.INTEGER_LITERAL):
                self.error_collector.add_error(
                    "parser",
                    self.filename,
                    self.current_token.line if self.current_token else 0,
                    self.current_token.column if self.current_token else 0,
                    "Expected array size (integer literal)"
                )
            else:
                array_size = int(self.current_token.value)
                self.advance()
            
            self.expect(TokenType.RBRACKET)
        
        # Check for initializer
        if self.match(TokenType.ASSIGN):
            self.advance()
            initializer = self.parse_expression()
        
        self.expect(TokenType.SEMICOLON)
        
        return VarDecl(var_type, name, array_size, line, column, initializer)

    # ============================================================
    # Statements
    # ============================================================
    
    def parse_statement(self) -> Optional[Statement]:
        """
        Parse statement
        
        Grammar:
            statement → var_decl | compound_stmt | if_stmt | while_stmt | for_stmt | return_stmt | expr_stmt
        """
        # Variable declaration
        if self.match(TokenType.INT, TokenType.CHAR):
            var_type = self.parse_type()
            if not self.match(TokenType.IDENTIFIER):
                self.error_collector.add_error(
                    "parser",
                    self.filename,
                    self.current_token.line if self.current_token else 0,
                    self.current_token.column if self.current_token else 0,
                    "Expected identifier in variable declaration"
                )
                self.synchronize()
                return None
            
            name = self.current_token.value
            line = self.current_token.line
            column = self.current_token.column
            self.advance()
            
            return self.parse_var_decl(var_type, name, line, column)
        
        # Compound statement
        if self.match(TokenType.LBRACE):
            return self.parse_compound_stmt()
        
        # If statement
        if self.match(TokenType.IF):
            return self.parse_if_stmt()
        
        # While statement
        if self.match(TokenType.WHILE):
            return self.parse_while_stmt()
        
        # For statement
        if self.match(TokenType.FOR):
            return self.parse_for_stmt()
        
        # Return statement
        if self.match(TokenType.RETURN):
            return self.parse_return_stmt()
        
        # Expression statement
        return self.parse_expr_stmt()
    
    def parse_compound_stmt(self) -> CompoundStmt:
        """Parse compound statement (block)"""
        line = self.current_token.line if self.current_token else 0
        column = self.current_token.column if self.current_token else 0
        
        self.expect(TokenType.LBRACE)
        
        statements = []
        while not self.match(TokenType.RBRACE) and not self.match(TokenType.EOF):
            stmt = self.parse_statement()
            if stmt:
                statements.append(stmt)
        
        self.expect(TokenType.RBRACE)
        
        return CompoundStmt(statements, line, column)
    
    def parse_if_stmt(self) -> IfStmt:
        """Parse if statement"""
        line = self.current_token.line if self.current_token else 0
        column = self.current_token.column if self.current_token else 0
        
        self.expect(TokenType.IF)
        self.expect(TokenType.LPAREN)
        
        condition = self.parse_expression()
        
        self.expect(TokenType.RPAREN)
        
        then_stmt = self.parse_statement()
        
        else_stmt = None
        if self.match(TokenType.ELSE):
            self.advance()
            else_stmt = self.parse_statement()
        
        return IfStmt(condition, then_stmt, else_stmt, line, column)
    
    def parse_while_stmt(self) -> WhileStmt:
        """Parse while statement"""
        line = self.current_token.line if self.current_token else 0
        column = self.current_token.column if self.current_token else 0
        
        self.expect(TokenType.WHILE)
        self.expect(TokenType.LPAREN)
        
        condition = self.parse_expression()
        
        self.expect(TokenType.RPAREN)
        
        body = self.parse_statement()
        
        return WhileStmt(condition, body, line, column)
    
    def parse_for_stmt(self) -> ForStmt:
        """Parse for statement"""
        line = self.current_token.line if self.current_token else 0
        column = self.current_token.column if self.current_token else 0
        
        self.expect(TokenType.FOR)
        self.expect(TokenType.LPAREN)
        
        # Init expression (optional)
        init = None
        if not self.match(TokenType.SEMICOLON):
            init = self.parse_expression()
        self.expect(TokenType.SEMICOLON)
        
        # Condition expression (optional)
        condition = None
        if not self.match(TokenType.SEMICOLON):
            condition = self.parse_expression()
        self.expect(TokenType.SEMICOLON)
        
        # Increment expression (optional)
        increment = None
        if not self.match(TokenType.RPAREN):
            increment = self.parse_expression()
        
        self.expect(TokenType.RPAREN)
        
        body = self.parse_statement()
        
        return ForStmt(init, condition, increment, body, line, column)
    
    def parse_return_stmt(self) -> ReturnStmt:
        """Parse return statement"""
        line = self.current_token.line if self.current_token else 0
        column = self.current_token.column if self.current_token else 0
        
        self.expect(TokenType.RETURN)
        
        # Return value (optional for void functions)
        value = None
        if not self.match(TokenType.SEMICOLON):
            value = self.parse_expression()
        
        self.expect(TokenType.SEMICOLON)
        
        return ReturnStmt(value, line, column)
    
    def parse_expr_stmt(self) -> ExprStmt:
        """Parse expression statement"""
        line = self.current_token.line if self.current_token else 0
        column = self.current_token.column if self.current_token else 0
        
        # Empty statement (just semicolon)
        if self.match(TokenType.SEMICOLON):
            self.advance()
            return ExprStmt(None, line, column)
        
        expr = self.parse_expression()
        self.expect(TokenType.SEMICOLON)
        
        return ExprStmt(expr, line, column)

    # ============================================================
    # Expressions (Operator Precedence Climbing)
    # ============================================================
    
    def parse_expression(self) -> Expression:
        """Parse expression (entry point)"""
        return self.parse_assignment()
    
    def parse_assignment(self) -> Expression:
        """
        Parse assignment expression
        Grammar: assignment → logical_or ('=' assignment)?
        """
        expr = self.parse_logical_or()
        
        if self.match(TokenType.ASSIGN):
            line = self.current_token.line
            column = self.current_token.column
            self.advance()
            value = self.parse_assignment()  # Right associative
            return Assignment(expr, value, line, column)
        
        return expr
    
    def parse_logical_or(self) -> Expression:
        """Parse logical OR (||)"""
        left = self.parse_logical_and()
        
        while self.match(TokenType.OR):
            op_token = self.current_token
            self.advance()
            right = self.parse_logical_and()
            left = BinaryOp('||', left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_logical_and(self) -> Expression:
        """Parse logical AND (&&)"""
        left = self.parse_bitwise_or()
        
        while self.match(TokenType.AND):
            op_token = self.current_token
            self.advance()
            right = self.parse_bitwise_or()
            left = BinaryOp('&&', left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_bitwise_or(self) -> Expression:
        """Parse bitwise OR (|)"""
        left = self.parse_bitwise_xor()
        
        while self.match(TokenType.BITOR):
            op_token = self.current_token
            self.advance()
            right = self.parse_bitwise_xor()
            left = BinaryOp('|', left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_bitwise_xor(self) -> Expression:
        """Parse bitwise XOR (^)"""
        left = self.parse_bitwise_and()
        
        while self.match(TokenType.BITXOR):
            op_token = self.current_token
            self.advance()
            right = self.parse_bitwise_and()
            left = BinaryOp('^', left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_bitwise_and(self) -> Expression:
        """Parse bitwise AND (&)"""
        left = self.parse_equality()
        
        while self.match(TokenType.BITAND):
            op_token = self.current_token
            self.advance()
            right = self.parse_equality()
            left = BinaryOp('&', left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_equality(self) -> Expression:
        """Parse equality (==, !=)"""
        left = self.parse_relational()
        
        while self.match(TokenType.EQ, TokenType.NE):
            op_token = self.current_token
            op = op_token.value
            self.advance()
            right = self.parse_relational()
            left = BinaryOp(op, left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_relational(self) -> Expression:
        """Parse relational (<, >, <=, >=)"""
        left = self.parse_shift()
        
        while self.match(TokenType.LT, TokenType.GT, TokenType.LE, TokenType.GE):
            op_token = self.current_token
            op = op_token.value
            self.advance()
            right = self.parse_shift()
            left = BinaryOp(op, left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_shift(self) -> Expression:
        """Parse shift (<<, >>)"""
        left = self.parse_additive()
        
        while self.match(TokenType.LSHIFT, TokenType.RSHIFT):
            op_token = self.current_token
            op = op_token.value
            self.advance()
            right = self.parse_additive()
            left = BinaryOp(op, left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_additive(self) -> Expression:
        """Parse additive (+, -)"""
        left = self.parse_multiplicative()
        
        while self.match(TokenType.PLUS, TokenType.MINUS):
            op_token = self.current_token
            op = op_token.value
            self.advance()
            right = self.parse_multiplicative()
            left = BinaryOp(op, left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_multiplicative(self) -> Expression:
        """Parse multiplicative (*, /, %)"""
        left = self.parse_unary()
        
        while self.match(TokenType.STAR, TokenType.SLASH, TokenType.PERCENT):
            op_token = self.current_token
            op = op_token.value
            self.advance()
            right = self.parse_unary()
            left = BinaryOp(op, left, right, op_token.line, op_token.column)
        
        return left
    
    def parse_unary(self) -> Expression:
        """
        Parse unary expression
        Grammar: unary → ('!' | '-' | '*' | '&') unary | postfix
        """
        if self.match(TokenType.NOT, TokenType.MINUS, TokenType.STAR, TokenType.BITAND):
            op_token = self.current_token
            op = op_token.value
            self.advance()
            operand = self.parse_unary()
            return UnaryOp(op, operand, op_token.line, op_token.column)
        
        return self.parse_postfix()
    
    def parse_postfix(self) -> Expression:
        """
        Parse postfix expression (array access, function call)
        Grammar: postfix → primary ('[' expression ']' | '(' arg_list? ')')*
        """
        expr = self.parse_primary()
        
        while True:
            # Array access
            if self.match(TokenType.LBRACKET):
                line = self.current_token.line
                column = self.current_token.column
                self.advance()
                index = self.parse_expression()
                self.expect(TokenType.RBRACKET)
                expr = ArrayAccess(expr, index, line, column)
            
            # Function call
            elif self.match(TokenType.LPAREN):
                line = self.current_token.line
                column = self.current_token.column
                self.advance()
                
                # Parse arguments
                args = []
                if not self.match(TokenType.RPAREN):
                    args = self.parse_arg_list()
                
                self.expect(TokenType.RPAREN)
                
                # Extract function name from expr (must be Identifier)
                if isinstance(expr, Identifier):
                    expr = FunctionCall(expr.name, args, line, column)
                else:
                    self.error_collector.add_error(
                        "parser",
                        self.filename,
                        line,
                        column,
                        "Function call requires identifier"
                    )
            else:
                break
        
        return expr
    
    def parse_arg_list(self) -> List[Expression]:
        """Parse function argument list"""
        args = []
        
        args.append(self.parse_expression())
        
        while self.match(TokenType.COMMA):
            self.advance()
            args.append(self.parse_expression())
        
        return args
    
    def parse_primary(self) -> Expression:
        """
        Parse primary expression
        Grammar: primary → IDENTIFIER | INTEGER_LITERAL | CHAR_LITERAL | '(' expression ')'
        """
        # Identifier
        if self.match(TokenType.IDENTIFIER):
            token = self.current_token
            self.advance()
            return Identifier(token.value, token.line, token.column)
        
        # Integer literal
        if self.match(TokenType.INTEGER_LITERAL):
            token = self.current_token
            self.advance()
            return IntegerLiteral(int(token.value), token.line, token.column)
        
        # Character literal
        if self.match(TokenType.CHAR_LITERAL):
            token = self.current_token
            self.advance()
            return CharLiteral(token.value, token.line, token.column)
        
        # String literal
        if self.match(TokenType.STRING_LITERAL):
            token = self.current_token
            self.advance()
            return StringLiteral(token.value, token.line, token.column)
        
        # Parenthesized expression
        if self.match(TokenType.LPAREN):
            self.advance()
            expr = self.parse_expression()
            self.expect(TokenType.RPAREN)
            return expr
        
        # Error: unexpected token
        self.error_collector.add_error(
            "parser",
            self.filename,
            self.current_token.line if self.current_token else 0,
            self.current_token.column if self.current_token else 0,
            f"Unexpected token in expression: {self.current_token.type.name if self.current_token else 'EOF'}"
        )
        
        # Return dummy expression to continue
        return IntegerLiteral(0, 
                            self.current_token.line if self.current_token else 0,
                            self.current_token.column if self.current_token else 0)
