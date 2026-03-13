"""
Linker Wrapper
Invokes ARM linker to create executable binary
"""

import subprocess
from pathlib import Path
from ...common.error import ErrorCollector


class Linker:
    """
    Wrapper for arm-linux-gnueabihf-ld linker
    """
    
    def __init__(self, error_collector: ErrorCollector):
        """
        Initialize linker
        
        Args:
            error_collector: Error collector for reporting linker errors
        """
        self.error_collector = error_collector
    
    def link(self, object_files: list, output_file: str, runtime_dir: Path) -> bool:
        """
        Link object files to create executable
        
        Args:
            object_files: List of object file paths
            output_file: Output executable path
            runtime_dir: Path to runtime directory (contains crt0.o, syscalls.o, app.ld)
        
        Returns:
            True if successful, False if errors
        """
        # Build runtime object files if needed
        crt0_obj = runtime_dir / 'crt0.o'
        syscalls_obj = runtime_dir / 'syscalls.o'
        divmod_obj = runtime_dir / 'divmod.o'
        reflibc_obj = runtime_dir / 'reflibc.o'
        linker_script = runtime_dir / 'app.ld'
        
        # Assemble runtime files if not already assembled
        if not crt0_obj.exists():
            self._assemble_runtime_file(runtime_dir / 'crt0.S', crt0_obj)
        
        if not syscalls_obj.exists():
            self._assemble_runtime_file(runtime_dir / 'syscalls.S', syscalls_obj)
        
        if not divmod_obj.exists():
            self._assemble_runtime_file(runtime_dir / 'divmod.S', divmod_obj)
        
        # Compile reflibc.c if not already compiled
        if not reflibc_obj.exists():
            self._compile_runtime_file(runtime_dir / 'reflibc.c', reflibc_obj)
        
        # Build linker command
        cmd = [
            'arm-linux-gnueabihf-ld',
            '-T', str(linker_script),
            str(crt0_obj),
            *object_files,
            str(reflibc_obj),
            str(syscalls_obj),
            str(divmod_obj),
            '-o', output_file
        ]
        
        # Invoke linker
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        # Check for errors
        if result.returncode != 0:
            for line in result.stderr.splitlines():
                self.error_collector.add_error(
                    "linker",
                    output_file,
                    0, 0,
                    line
                )
            return False
        
        return True
    
    def _assemble_runtime_file(self, source: Path, output: Path) -> bool:
        """
        Assemble runtime source file
        
        Args:
            source: Source assembly file
            output: Output object file
        
        Returns:
            True if successful, False if errors
        """
        result = subprocess.run(
            ['arm-linux-gnueabihf-as', '-mcpu=cortex-a8', '-o', str(output), str(source)],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            for line in result.stderr.splitlines():
                self.error_collector.add_error(
                    "assembler",
                    str(source),
                    0, 0,
                    line
                )
            return False
        
        return True
    
    def _compile_runtime_file(self, source: Path, output: Path) -> bool:
        """
        Compile runtime C source file to object file
        
        Args:
            source: Source C file
            output: Output object file
        
        Returns:
            True if successful, False if errors
        """
        result = subprocess.run(
            [
                'arm-linux-gnueabihf-gcc',
                '-c',                    # Compile only, don't link
                '-mcpu=cortex-a8',       # Target Cortex-A8 (BeagleBone Black)
                '-marm',                 # Use ARM instruction set (not Thumb)
                '-O2',                   # Optimize for size/speed
                '-fno-builtin',          # Don't use GCC builtins
                '-nostdlib',             # Don't link standard library
                '-ffreestanding',        # Freestanding environment
                '-o', str(output),       # Output object file
                str(source)              # Input C source file
            ],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            for line in result.stderr.splitlines():
                self.error_collector.add_error(
                    "compiler",
                    str(source),
                    0, 0,
                    line
                )
            return False
        
        return True
