"""
Assembler Wrapper
Invokes ARM assembler to convert assembly to object file
"""

import subprocess
import tempfile
from pathlib import Path
from ...common.error import ErrorCollector


class Assembler:
    """
    Wrapper for arm-linux-gnueabihf-as assembler
    """
    
    def __init__(self, error_collector: ErrorCollector):
        """
        Initialize assembler
        
        Args:
            error_collector: Error collector for reporting assembler errors
        """
        self.error_collector = error_collector
    
    def assemble(self, assembly: str, output_file: str, filename: str = "input.s") -> bool:
        """
        Assemble assembly code to object file
        
        Args:
            assembly: Assembly code as string
            output_file: Output object file path
            filename: Source filename (for error reporting)
        
        Returns:
            True if successful, False if errors
        """
        # Write assembly to temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.s', delete=False) as f:
            f.write(assembly)
            temp_asm_file = f.name
        
        try:
            # Invoke assembler
            result = subprocess.run(
                ['arm-linux-gnueabihf-as', '-mcpu=cortex-a8', '-o', output_file, temp_asm_file],
                capture_output=True,
                text=True
            )
            
            # Check for errors
            if result.returncode != 0:
                # Parse assembler errors
                for line in result.stderr.splitlines():
                    self.error_collector.add_error(
                        "assembler",
                        filename,
                        0, 0,
                        line
                    )
                return False
            
            return True
        
        finally:
            # Clean up temporary file
            Path(temp_asm_file).unlink(missing_ok=True)
