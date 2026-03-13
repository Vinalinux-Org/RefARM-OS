"""
Lexical Analyzer (Lexer)
Tokenizes Subset C source code
"""

from typing import List, Optional
from .token import Token, TokenType, KEYWORDS
from ...common.error import ErrorCollector


class Lexer:
    """
    Lexical analyzer for Subset C
    Converts source text into stream of tokens
    """
    
    def __init__(self, source: str, filename: str, error_collector: ErrorCollector):
        """
        Initialize lexer
        
        Args:
            source: Source code text
            filename: Source file name (for error reporting)
            error_collector: Error collector for reporting lexical errors
        """
        self.source = source
        self.filename = filename
        self.error_collector = error_collector
        
        # Current position in source
        self.pos = 0
        self.line = 1
        self.column = 1
        
        # Current character
        self.current_char: Optional[str] = self.source[0] if source else None
    
    def advance(self) -> None:
        """Move to next character, update line and column"""
        if self.current_char == '\n':
            self.line += 1
            self.column = 1
        else:
            self.column += 1
        
        self.pos += 1
        if self.pos < len(self.source):
            self.current_char = self.source[self.pos]
        else:
            self.current_char = None
    
    def peek(self, offset: int = 1) -> Optional[str]:
        """Look ahead at character without advancing"""
        peek_pos = self.pos + offset
        if peek_pos < len(self.source):
            return self.source[peek_pos]
        return None
    
    def skip_whitespace(self) -> None:
        """Skip whitespace characters"""
        while self.current_char and self.current_char.isspace():
            self.advance()
    
    def skip_line_comment(self) -> None:
        """Skip single-line comment (// ...)"""
        # Skip //
        self.advance()
        self.advance()
        
        # Skip until newline or EOF
        while self.current_char and self.current_char != '\n':
            self.advance()
    
    def skip_block_comment(self) -> None:
        """Skip multi-line comment (/* ... */)"""
        start_line = self.line
        start_column = self.column
        
        # Skip /*
        self.advance()
        self.advance()
        
        # Skip until */ or EOF
        while self.current_char:
            if self.current_char == '*' and self.peek() == '/':
                # Found closing */
                self.advance()  # Skip *
                self.advance()  # Skip /
                return
            self.advance()
        
        # Reached EOF without closing */
        self.error_collector.add_error(
            "lexer",
            self.filename,
            start_line,
            start_column,
            "Unterminated block comment"
        )
    
    def read_identifier(self) -> str:
        """Read identifier or keyword"""
        start_pos = self.pos
        
        # First character: letter or underscore
        # Following characters: letter, digit, or underscore
        while self.current_char and (self.current_char.isalnum() or self.current_char == '_'):
            self.advance()
        
        return self.source[start_pos:self.pos]
    
    def read_number(self) -> str:
        """Read integer literal (decimal only)"""
        start_pos = self.pos
        
        while self.current_char and self.current_char.isdigit():
            self.advance()
        
        return self.source[start_pos:self.pos]
    
    def read_char_literal(self) -> Optional[str]:
        """Read character literal ('c')"""
        start_line = self.line
        start_column = self.column
        
        # Skip opening '
        self.advance()
        
        if not self.current_char:
            self.error_collector.add_error(
                "lexer",
                self.filename,
                start_line,
                start_column,
                "Unterminated character literal"
            )
            return None
        
        # Read character
        char = self.current_char
        self.advance()
        
        # Check for closing '
        if self.current_char != "'":
            self.error_collector.add_error(
                "lexer",
                self.filename,
                start_line,
                start_column,
                "Unterminated character literal (missing closing ')"
            )
            return None
        
        # Skip closing '
        self.advance()
        
        return char
    
    def read_string_literal(self) -> Optional[str]:
        """
        Read string literal: "hello world"
        Returns the string value or None if error
        """
        start_line = self.line
        start_column = self.column
        
        # Skip opening "
        self.advance()
        
        string_value = ""
        
        while self.current_char and self.current_char != '"':
            if self.current_char == '\\':
                # Escape sequence
                self.advance()
                if not self.current_char:
                    self.error_collector.add_error(
                        "lexer", self.filename, start_line, start_column,
                        "Unterminated string literal"
                    )
                    return None
                
                escape_chars = {
                    'n': '\n',
                    't': '\t',
                    'r': '\r',
                    '\\': '\\',
                    "'": "'",
                    '"': '"',
                    '0': '\0'
                }
                
                if self.current_char in escape_chars:
                    string_value += escape_chars[self.current_char]
                else:
                    string_value += self.current_char
                
                self.advance()
            else:
                string_value += self.current_char
                self.advance()
        
        # Expect closing quote
        if self.current_char != '"':
            self.error_collector.add_error(
                "lexer", self.filename, start_line, start_column,
                "Unterminated string literal"
            )
            return None
        
        self.advance()  # Skip closing quote
        return string_value
    
    def skip_preprocessor_directive(self) -> None:
        """
        Skip preprocessor directive (line starting with #)
        """
        while self.current_char and self.current_char != '\n':
            self.advance()
        if self.current_char == '\n':
            self.advance()
    
    def tokenize(self) -> List[Token]:
        """
        Tokenize entire source code
        Returns list of tokens including EOF token
        """
        tokens: List[Token] = []
        
        while self.current_char:
            # Save position for token
            token_line = self.line
            token_column = self.column
            
            # Skip whitespace
            if self.current_char.isspace():
                self.skip_whitespace()
                continue
            
            # Skip preprocessor directives
            if self.current_char == '#':
                self.skip_preprocessor_directive()
                continue

            # Comments
            if self.current_char == '/' and self.peek() == '/':
                self.skip_line_comment()
                continue
            
            if self.current_char == '/' and self.peek() == '*':
                self.skip_block_comment()
                continue
            
            # Identifiers and keywords
            if self.current_char.isalpha() or self.current_char == '_':
                identifier = self.read_identifier()
                # Check if keyword
                token_type = KEYWORDS.get(identifier, TokenType.IDENTIFIER)
                tokens.append(Token(token_type, identifier, token_line, token_column))
                continue
            
            # Numbers
            if self.current_char.isdigit():
                number = self.read_number()
                tokens.append(Token(TokenType.INTEGER_LITERAL, number, token_line, token_column))
                continue
            
            # Character literals
            if self.current_char == "'":
                char = self.read_char_literal()
                if char is not None:
                    tokens.append(Token(TokenType.CHAR_LITERAL, char, token_line, token_column))
                continue
            
            # String literals
            if self.current_char == '"':
                string = self.read_string_literal()
                if string is not None:
                    tokens.append(Token(TokenType.STRING_LITERAL, string, token_line, token_column))
                continue
            
            # Two-character operators
            two_char = self.current_char + (self.peek() or '')
            if two_char == '==':
                tokens.append(Token(TokenType.EQ, '==', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '!=':
                tokens.append(Token(TokenType.NE, '!=', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '<=':
                tokens.append(Token(TokenType.LE, '<=', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '>=':
                tokens.append(Token(TokenType.GE, '>=', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '&&':
                tokens.append(Token(TokenType.AND, '&&', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '||':
                tokens.append(Token(TokenType.OR, '||', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '<<':
                tokens.append(Token(TokenType.LSHIFT, '<<', token_line, token_column))
                self.advance()
                self.advance()
                continue
            elif two_char == '>>':
                tokens.append(Token(TokenType.RSHIFT, '>>', token_line, token_column))
                self.advance()
                self.advance()
                continue
            
            # Single-character tokens
            char = self.current_char
            if char == '+':
                tokens.append(Token(TokenType.PLUS, '+', token_line, token_column))
                self.advance()
            elif char == '-':
                tokens.append(Token(TokenType.MINUS, '-', token_line, token_column))
                self.advance()
            elif char == '*':
                tokens.append(Token(TokenType.STAR, '*', token_line, token_column))
                self.advance()
            elif char == '/':
                tokens.append(Token(TokenType.SLASH, '/', token_line, token_column))
                self.advance()
            elif char == '%':
                tokens.append(Token(TokenType.PERCENT, '%', token_line, token_column))
                self.advance()
            elif char == '<':
                tokens.append(Token(TokenType.LT, '<', token_line, token_column))
                self.advance()
            elif char == '>':
                tokens.append(Token(TokenType.GT, '>', token_line, token_column))
                self.advance()
            elif char == '!':
                tokens.append(Token(TokenType.NOT, '!', token_line, token_column))
                self.advance()
            elif char == '&':
                tokens.append(Token(TokenType.BITAND, '&', token_line, token_column))
                self.advance()
            elif char == '|':
                tokens.append(Token(TokenType.BITOR, '|', token_line, token_column))
                self.advance()
            elif char == '^':
                tokens.append(Token(TokenType.BITXOR, '^', token_line, token_column))
                self.advance()
            elif char == '=':
                tokens.append(Token(TokenType.ASSIGN, '=', token_line, token_column))
                self.advance()
            elif char == '(':
                tokens.append(Token(TokenType.LPAREN, '(', token_line, token_column))
                self.advance()
            elif char == ')':
                tokens.append(Token(TokenType.RPAREN, ')', token_line, token_column))
                self.advance()
            elif char == '{':
                tokens.append(Token(TokenType.LBRACE, '{', token_line, token_column))
                self.advance()
            elif char == '}':
                tokens.append(Token(TokenType.RBRACE, '}', token_line, token_column))
                self.advance()
            elif char == '[':
                tokens.append(Token(TokenType.LBRACKET, '[', token_line, token_column))
                self.advance()
            elif char == ']':
                tokens.append(Token(TokenType.RBRACKET, ']', token_line, token_column))
                self.advance()
            elif char == ';':
                tokens.append(Token(TokenType.SEMICOLON, ';', token_line, token_column))
                self.advance()
            elif char == ',':
                tokens.append(Token(TokenType.COMMA, ',', token_line, token_column))
                self.advance()
            else:
                # Invalid character
                self.error_collector.add_error(
                    "lexer",
                    self.filename,
                    token_line,
                    token_column,
                    f"Invalid character: '{char}'"
                )
                self.advance()
        
        # Add EOF token
        tokens.append(Token(TokenType.EOF, "", self.line, self.column))
        
        return tokens
