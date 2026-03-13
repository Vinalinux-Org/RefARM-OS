"""
Error Reporting System
Centralized error handling cho compiler
"""

import sys
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class CompilerError:
    """
    Represents a compiler error with location information
    
    Attributes:
        phase: Compiler phase where error occurred (lexer, parser, semantic, codegen)
        filename: Source file name
        line: Line number (1-indexed)
        column: Column number (1-indexed)
        message: Error description
    """
    phase: str
    filename: str
    line: int
    column: int
    message: str
    
    def __str__(self) -> str:
        """Format error message: <filename>:<line>:<column>: <phase> error: <message>"""
        return f"{self.filename}:{self.line}:{self.column}: {self.phase} error: {self.message}"


class ErrorCollector:
    """
    Collects and reports compiler errors from all phases
    Allows multiple errors to be reported before stopping compilation
    """
    
    def __init__(self):
        self.errors: List[CompilerError] = []
    
    def add_error(
        self,
        phase: str,
        filename: str,
        line: int,
        column: int,
        message: str
    ) -> None:
        """Add a compiler error to the collection"""
        error = CompilerError(phase, filename, line, column, message)
        self.errors.append(error)
    
    def has_errors(self) -> bool:
        """Check if any errors have been collected"""
        return len(self.errors) > 0
    
    def error_count(self) -> int:
        """Return number of errors collected"""
        return len(self.errors)
    
    def report_all(self, file=sys.stderr) -> None:
        """
        Report all collected errors to stderr (or specified file)
        Errors are sorted by line and column for better readability
        """
        # Sort errors by line, then column
        sorted_errors = sorted(self.errors, key=lambda e: (e.line, e.column))
        
        for error in sorted_errors:
            print(error, file=file)
    
    def clear(self) -> None:
        """Clear all collected errors"""
        self.errors.clear()
    
    def get_errors(self) -> List[CompilerError]:
        """Return list of all collected errors"""
        return self.errors.copy()
