"""
Simple Preprocessor for VinixOS Compiler
Handles #include directives
"""

import os
from pathlib import Path
from typing import List
from ...common.error import ErrorCollector


class Preprocessor:
    def __init__(self, error_collector: ErrorCollector):
        self.error_collector = error_collector
        self.include_paths = [
            Path(__file__).parent.parent.parent / "runtime",  # toolchain/runtime/
        ]
        self.processed_files = set()  # Prevent circular includes
    
    def process(self, source_code: str, filename: str) -> str:
        """
        Process source code and handle #include directives
        
        Args:
            source_code: Input C source code
            filename: Source file name for error reporting
            
        Returns:
            Processed source code with includes expanded
        """
        lines = source_code.split('\n')
        processed_lines = []
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip()
            
            if stripped.startswith('#include'):
                # Parse #include directive
                include_content = self._process_include(stripped, filename, line_num)
                if include_content:
                    processed_lines.append(f"// Included from {stripped}")
                    processed_lines.append(include_content)
                    processed_lines.append(f"// End include {stripped}")
                else:
                    # Keep original line if include failed
                    processed_lines.append(line)
            else:
                processed_lines.append(line)
        
        return '\n'.join(processed_lines)
    
    def _process_include(self, include_line: str, filename: str, line_num: int) -> str:
        """
        Process a single #include directive
        
        Args:
            include_line: The #include line (e.g., '#include "reflibc.h"')
            filename: Source file name for error reporting
            line_num: Line number for error reporting
            
        Returns:
            Content of included file, or empty string if error
        """
        # Parse include directive
        # Support: #include "file.h" and #include <file.h>
        include_line = include_line.strip()
        
        if '"' in include_line:
            # #include "file.h"
            start = include_line.find('"')
            end = include_line.rfind('"')
            if start != -1 and end != -1 and start < end:
                header_name = include_line[start+1:end]
            else:
                self.error_collector.add_error("preprocessor", filename, line_num, 1, 
                                             f"Invalid #include syntax: {include_line}")
                return ""
        elif '<' in include_line and '>' in include_line:
            # #include <file.h>
            start = include_line.find('<')
            end = include_line.find('>')
            if start != -1 and end != -1 and start < end:
                header_name = include_line[start+1:end]
            else:
                self.error_collector.add_error("preprocessor", filename, line_num, 1,
                                             f"Invalid #include syntax: {include_line}")
                return ""
        else:
            self.error_collector.add_error("preprocessor", filename, line_num, 1,
                                         f"Invalid #include syntax: {include_line}")
            return ""
        
        # Find header file
        header_path = self._find_header(header_name)
        if not header_path:
            self.error_collector.add_error("preprocessor", filename, line_num, 1,
                                         f"Header file not found: {header_name}")
            return ""
        
        # Prevent circular includes
        if str(header_path) in self.processed_files:
            return f"// Circular include prevented: {header_name}"
        
        # Read header file
        try:
            with open(header_path, 'r') as f:
                header_content = f.read()
            
            # Mark as processed
            self.processed_files.add(str(header_path))
            
            # Recursively process includes in header file
            processed_content = self.process(header_content, str(header_path))
            
            return processed_content
            
        except IOError as e:
            self.error_collector.add_error("preprocessor", filename, line_num, 1,
                                         f"Cannot read header file {header_name}: {e}")
            return ""
    
    def _find_header(self, header_name: str) -> Path:
        """
        Find header file in include paths
        
        Args:
            header_name: Name of header file (e.g., "reflibc.h")
            
        Returns:
            Path to header file, or None if not found
        """
        for include_path in self.include_paths:
            header_path = include_path / header_name
            if header_path.exists():
                return header_path
        
        return None
    
    def add_include_path(self, path: str):
        """Add additional include path"""
        self.include_paths.append(Path(path))