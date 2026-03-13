"""
Compiler Configuration
Stores compiler options and settings
"""

from dataclasses import dataclass
from pathlib import Path


@dataclass
class CompilerConfig:
    """
    Compiler configuration and options
    
    Attributes:
        input_file: Input source file path
        output_file: Output file path
        emit_tokens: Debug flag to dump token stream
        emit_ast: Debug flag to dump AST
        emit_ir: Debug flag to dump IR
        emit_asm: Flag to output assembly only (no assemble/link)
        optimization_level: Optimization level (0 = no optimization)
        target_arch: Target architecture (armv7-a)
        syscall_abi: Syscall ABI (refarm)
        assembler: Path to assembler executable
        linker: Path to linker executable
        linker_script: Path to linker script
    """
    
    # Input/Output
    input_file: str
    output_file: str = "a.out"
    
    # Debug options
    emit_tokens: bool = False
    emit_ast: bool = False
    emit_ir: bool = False
    emit_asm: bool = False  # -S flag: assembly only
    
    # Compilation options
    optimization_level: int = 0
    
    # Target configuration
    target_arch: str = "armv7-a"
    syscall_abi: str = "refarm"
    
    # External tools
    assembler: str = "arm-linux-gnueabihf-as"
    linker: str = "arm-linux-gnueabihf-ld"
    linker_script: str = "runtime/app.ld"
    
    @property
    def input_path(self) -> Path:
        """Return input file as Path object"""
        return Path(self.input_file)
    
    @property
    def output_path(self) -> Path:
        """Return output file as Path object"""
        return Path(self.output_file)
    
    def __str__(self) -> str:
        """String representation for debugging"""
        return (
            f"CompilerConfig(\n"
            f"  input: {self.input_file}\n"
            f"  output: {self.output_file}\n"
            f"  emit_tokens: {self.emit_tokens}\n"
            f"  emit_ast: {self.emit_ast}\n"
            f"  emit_ir: {self.emit_ir}\n"
            f"  emit_asm: {self.emit_asm}\n"
            f"  optimization: {self.optimization_level}\n"
            f"  target: {self.target_arch}\n"
            f")"
        )
