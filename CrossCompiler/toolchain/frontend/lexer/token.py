"""
Token Definitions
Defines token types and Token dataclass for lexical analysis
"""

from enum import Enum
from dataclasses import dataclass


class TokenType(Enum):
    """Token types for Subset C language"""
    
    # Keywords
    INT = "int"
    CHAR = "char"
    IF = "if"
    ELSE = "else"
    WHILE = "while"
    FOR = "for"
    RETURN = "return"
    VOID = "void"
    
    # Identifiers and Literals
    IDENTIFIER = "IDENTIFIER"
    INTEGER_LITERAL = "INTEGER_LITERAL"
    CHAR_LITERAL = "CHAR_LITERAL"
    STRING_LITERAL = "STRING_LITERAL"
    
    # Arithmetic Operators
    PLUS = "+"
    MINUS = "-"
    STAR = "*"
    SLASH = "/"
    PERCENT = "%"
    
    # Comparison Operators
    EQ = "=="
    NE = "!="
    LT = "<"
    GT = ">"
    LE = "<="
    GE = ">="
    
    # Logical Operators
    AND = "&&"
    OR = "||"
    NOT = "!"
    
    # Bitwise Operators
    BITAND = "&"
    BITOR = "|"
    BITXOR = "^"
    LSHIFT = "<<"
    RSHIFT = ">>"
    
    # Assignment
    ASSIGN = "="
    
    # Delimiters
    LPAREN = "("
    RPAREN = ")"
    LBRACE = "{"
    RBRACE = "}"
    LBRACKET = "["
    RBRACKET = "]"
    SEMICOLON = ";"
    COMMA = ","
    
    # Special
    EOF = "EOF"


# Keywords mapping for quick lookup
KEYWORDS = {
    "int": TokenType.INT,
    "char": TokenType.CHAR,
    "if": TokenType.IF,
    "else": TokenType.ELSE,
    "while": TokenType.WHILE,
    "for": TokenType.FOR,
    "return": TokenType.RETURN,
    "void": TokenType.VOID,
}


@dataclass
class Token:
    """
    Represents a single token from source code
    
    Attributes:
        type: Token type (TokenType enum)
        value: Actual text from source (lexeme)
        line: Line number in source (1-indexed)
        column: Column number in source (1-indexed)
    """
    type: TokenType
    value: str
    line: int
    column: int
    
    def __repr__(self) -> str:
        """String representation for debugging"""
        return f"Token({self.type.name}, '{self.value}', {self.line}:{self.column})"
    
    def __str__(self) -> str:
        """Human-readable string representation"""
        if self.type in (TokenType.IDENTIFIER, TokenType.INTEGER_LITERAL, TokenType.CHAR_LITERAL, TokenType.STRING_LITERAL):
            return f"{self.type.name}({self.value})"
        else:
            return self.type.name
