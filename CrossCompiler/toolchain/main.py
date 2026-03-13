#!/usr/bin/env python3
"""
Phase 2 Compiler - Main Entry Point
Cross-compiler cho Subset C targeting ARMv7-A (VinixOS Platform)
"""

import sys
import argparse
from pathlib import Path

from .common.config import CompilerConfig
from .common.error import ErrorCollector


def print_ast(node, indent=0):
    """Pretty print AST for debugging"""
    prefix = "  " * indent
    
    if node is None:
        print(f"{prefix}None")
        return
    
    node_type = type(node).__name__
    print(f"{prefix}{node_type}", end="")
    
    # Print relevant attributes
    if hasattr(node, 'name'):
        print(f"(name={node.name})", end="")
    if hasattr(node, 'value'):
        print(f"(value={node.value})", end="")
    if hasattr(node, 'operator'):
        print(f"(op={node.operator})", end="")
    
    print()
    
    # Recursively print children
    if hasattr(node, 'declarations'):
        for decl in node.declarations:
            print_ast(decl, indent + 1)
    if hasattr(node, 'body') and node.body:
        print_ast(node.body, indent + 1)
    if hasattr(node, 'statements'):
        for stmt in node.statements:
            print_ast(stmt, indent + 1)
    if hasattr(node, 'condition') and node.condition:
        print_ast(node.condition, indent + 1)
    if hasattr(node, 'then_stmt') and node.then_stmt:
        print_ast(node.then_stmt, indent + 1)
    if hasattr(node, 'else_stmt') and node.else_stmt:
        print_ast(node.else_stmt, indent + 1)
    if hasattr(node, 'expression') and node.expression:
        print_ast(node.expression, indent + 1)
    if hasattr(node, 'left') and node.left:
        print_ast(node.left, indent + 1)
    if hasattr(node, 'right') and node.right:
        print_ast(node.right, indent + 1)
    if hasattr(node, 'operand') and node.operand:
        print_ast(node.operand, indent + 1)
    if hasattr(node, 'target') and node.target:
        print_ast(node.target, indent + 1)
    if hasattr(node, 'arguments'):
        for arg in node.arguments:
            print_ast(arg, indent + 1)


def parse_arguments() -> CompilerConfig:
    """Parse command-line arguments and return CompilerConfig"""
    parser = argparse.ArgumentParser(
        prog="vincc",
        description="Subset C Compiler for ARMv7-A (VinixOS Platform)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    
    parser.add_argument(
        "input",
        type=str,
        help="Input source file (.c)",
    )
    
    parser.add_argument(
        "-o", "--output",
        type=str,
        default="a.out",
        help="Output file (default: a.out)",
    )
    
    parser.add_argument(
        "-S",
        action="store_true",
        help="Compile to assembly only (no assemble/link)",
    )
    
    parser.add_argument(
        "--dump-tokens",
        action="store_true",
        help="Dump token stream (debug)",
    )
    
    parser.add_argument(
        "--dump-ast",
        action="store_true",
        help="Dump abstract syntax tree (debug)",
    )
    
    parser.add_argument(
        "--dump-ir",
        action="store_true",
        help="Dump intermediate representation (debug)",
    )
    
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s 0.1.0",
    )
    
    args = parser.parse_args()
    
    # Create CompilerConfig from arguments
    config = CompilerConfig(
        input_file=args.input,
        output_file=args.output,
        emit_tokens=args.dump_tokens,
        emit_ast=args.dump_ast,
        emit_ir=args.dump_ir,
        emit_asm=args.S,
    )
    
    return config


def main():
    """Main compiler driver"""
    # Parse arguments
    config = parse_arguments()
    
    # Create error collector
    error_collector = ErrorCollector()
    
    # Verify input file exists
    if not config.input_path.exists():
        print(f"Error: Input file '{config.input_file}' not found", file=sys.stderr)
        return 1
    
    # Read source code
    try:
        with open(config.input_path, "r", encoding="utf-8") as f:
            source_code = f.read()
    except Exception as e:
        print(f"Error reading input file: {e}", file=sys.stderr)
        return 1
    
    print(f"Compiling {config.input_file}...")
    
    # Phase 0: Preprocessing
    from .frontend.preprocessor.preprocessor import Preprocessor
    
    preprocessor = Preprocessor(error_collector)
    source_code = preprocessor.process(source_code, config.input_file)
    
    if config.emit_tokens:
        print("=== PREPROCESSED SOURCE ===")
        print(source_code)
        print("=== END PREPROCESSED SOURCE ===")
        print()
    
    # Phase 1: Lexical Analysis
    from .frontend.lexer.lexer import Lexer
    
    lexer = Lexer(source_code, config.input_file, error_collector)
    tokens = lexer.tokenize()
    
    # Check for lexical errors
    if error_collector.has_errors():
        error_collector.report_all()
        return 1
    
    # Debug: dump tokens if requested
    if config.emit_tokens:
        print("\n=== TOKENS ===")
        for token in tokens:
            print(token)
        print()
    
    print(f"Lexical analysis complete: {len(tokens)} tokens")
    
    # Phase 2: Syntax Analysis (Parsing)
    from .frontend.parser.parser import Parser
    
    parser = Parser(tokens, config.input_file, error_collector)
    ast = parser.parse()
    
    # Check for syntax errors
    if error_collector.has_errors():
        error_collector.report_all()
        return 1
    
    # Debug: dump AST if requested
    if config.emit_ast:
        print("\n=== AST ===")
        print_ast(ast)
        print()
    
    print(f"Syntax analysis complete")
    
    # Phase 3: Semantic Analysis
    from .frontend.semantic.semantic_analyzer import SemanticAnalyzer
    
    semantic_analyzer = SemanticAnalyzer(config.input_file, error_collector)
    success = semantic_analyzer.analyze(ast)
    
    # Check for semantic errors
    if error_collector.has_errors():
        error_collector.report_all()
        return 1
    
    print(f"Semantic analysis complete")
    
    # Phase 4: IR Generation
    from .middleend.ir.ir_generator import IRGenerator
    
    ir_generator = IRGenerator()
    ir_instructions = ir_generator.generate(ast)
    
    # Debug: dump IR if requested
    if config.emit_ir:
        print("\n=== IR ===")
        print(ir_generator.dump())
        print()
    
    print(f"IR generation complete: {len(ir_instructions)} instructions")
    
    # Phase 5: Code Generation
    from .backend.armv7a.code_generator import CodeGenerator
    
    code_generator = CodeGenerator()
    assembly = code_generator.generate(ir_instructions)
    
    print(f"Code generation complete")
    
    # If -S flag, write assembly to file and stop
    if config.emit_asm:
        asm_file = config.output_path.with_suffix('.s')
        with open(asm_file, 'w') as f:
            f.write(assembly)
        print(f"Assembly written to {asm_file}")
        return 0
    
    # Phase 6: Assemble
    from .backend.armv7a.assembler import Assembler
    import tempfile
    
    assembler = Assembler(error_collector)
    
    # Create temporary object file
    with tempfile.NamedTemporaryFile(suffix='.o', delete=False) as f:
        obj_file = f.name
    
    success = assembler.assemble(assembly, obj_file, config.input_file)
    
    if not success or error_collector.has_errors():
        error_collector.report_all()
        Path(obj_file).unlink(missing_ok=True)
        return 1
    
    print(f"Assembly complete")
    
    # Phase 7: Link
    from .backend.armv7a.linker import Linker
    
    linker = Linker(error_collector)
    runtime_dir = Path(__file__).parent / 'runtime'
    
    success = linker.link([obj_file], config.output_file, runtime_dir)
    
    # Clean up temporary object file
    Path(obj_file).unlink(missing_ok=True)
    
    if not success or error_collector.has_errors():
        error_collector.report_all()
        return 1
    
    print(f"Linking complete")
    print(f"Executable created: {config.output_file}")
    
    # Report any errors
    if error_collector.has_errors():
        error_collector.report_all()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
