"""
Integration tests for Phase 2 Compiler
Tests end-to-end compilation of various programs
"""

import subprocess
import pytest
from pathlib import Path


COMPILER_ROOT = Path(__file__).parent.parent
TEST_PROGRAMS_DIR = COMPILER_ROOT / 'tests' / 'integration' / 'programs'


def compile_program(source_file: str, output_file: str = None) -> tuple:
    """
    Compile a test program
    
    Returns:
        (return_code, stdout, stderr)
    """
    cmd = ['python3', '-m', 'toolchain.main']
    
    if output_file:
        cmd.extend(['-o', output_file])
    
    cmd.append(str(TEST_PROGRAMS_DIR / source_file))
    
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
    return result.returncode, result.stdout, result.stderr


class TestCompilation:
    """Test successful compilation of valid programs"""
    
    def test_hello_world(self):
        """Test hello world program compilation"""
        rc, stdout, stderr = compile_program('hello_world.c', 'test_hello')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_hello').exists()
        Path('test_hello').unlink(missing_ok=True)
    
    def test_factorial(self):
        """Test recursive factorial compilation"""
        rc, stdout, stderr = compile_program('test_lexer.c', 'test_fact')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_fact').exists()
        Path('test_fact').unlink(missing_ok=True)
    
    def test_multi_function(self):
        """Test multiple function calls"""
        rc, stdout, stderr = compile_program('multi_function.c', 'test_multi')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_multi').exists()
        Path('test_multi').unlink(missing_ok=True)
    
    def test_arrays(self):
        """Test array operations"""
        rc, stdout, stderr = compile_program('array_operations.c', 'test_arr')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_arr').exists()
        Path('test_arr').unlink(missing_ok=True)
    
    def test_pointers(self):
        """Test pointer arithmetic"""
        rc, stdout, stderr = compile_program('pointer_arithmetic.c', 'test_ptr')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_ptr').exists()
        Path('test_ptr').unlink(missing_ok=True)
    
    def test_control_flow(self):
        """Test nested control flow"""
        rc, stdout, stderr = compile_program('control_flow_nested.c', 'test_cf')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_cf').exists()
        Path('test_cf').unlink(missing_ok=True)
    
    def test_syscalls(self):
        """Test all syscalls"""
        rc, stdout, stderr = compile_program('all_syscalls.c', 'test_sys')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_sys').exists()
        Path('test_sys').unlink(missing_ok=True)
    
    def test_operators(self):
        """Test all operators"""
        rc, stdout, stderr = compile_program('test_parser_operators.c', 'test_op')
        assert rc == 0, f"Compilation failed: {stderr}"
        assert Path('test_op').exists()
        Path('test_op').unlink(missing_ok=True)


class TestDebugOptions:
    """Test compiler debug options"""
    
    def test_dump_tokens(self):
        """Test --dump-tokens option"""
        cmd = ['python3', '-m', 'toolchain.main', '--dump-tokens', 
               str(TEST_PROGRAMS_DIR / 'hello.c')]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode == 0
        assert '=== TOKENS ===' in result.stdout
    
    def test_dump_ast(self):
        """Test --dump-ast option"""
        cmd = ['python3', '-m', 'toolchain.main', '--dump-ast',
               str(TEST_PROGRAMS_DIR / 'hello.c')]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode == 0
        assert '=== AST ===' in result.stdout
    
    def test_dump_ir(self):
        """Test --dump-ir option"""
        cmd = ['python3', '-m', 'toolchain.main', '--dump-ir',
               str(TEST_PROGRAMS_DIR / 'hello.c')]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode == 0
        assert '=== IR ===' in result.stdout
    
    def test_assembly_output(self):
        """Test -S option (assembly output)"""
        cmd = ['python3', '-m', 'toolchain.main', '-S', '-o', 'test_asm.s',
               str(TEST_PROGRAMS_DIR / 'hello.c')]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode == 0
        assert Path('test_asm.s').exists()
        Path('test_asm.s').unlink(missing_ok=True)



class TestErrorDetection:
    """Test compiler error detection and reporting"""
    
    def test_lexical_error_invalid_char(self, tmp_path):
        """Test detection of invalid characters"""
        test_file = tmp_path / "invalid_char.c"
        test_file.write_text("int main() { int x = 5 @ 3; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_lexical_error_unterminated_string(self, tmp_path):
        """Test detection of unterminated character literal"""
        test_file = tmp_path / "unterminated.c"
        test_file.write_text("int main() { char c = 'a; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_syntax_error_missing_semicolon(self, tmp_path):
        """Test detection of missing semicolon"""
        test_file = tmp_path / "missing_semi.c"
        test_file.write_text("int main() { int x = 5 return 0; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_syntax_error_unmatched_brace(self, tmp_path):
        """Test detection of unmatched braces"""
        test_file = tmp_path / "unmatched.c"
        test_file.write_text("int main() { int x = 5;")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_semantic_error_undeclared_variable(self, tmp_path):
        """Test detection of undeclared variable"""
        test_file = tmp_path / "undeclared.c"
        test_file.write_text("int main() { x = 5; return 0; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_semantic_error_type_mismatch(self, tmp_path):
        """Test detection of type mismatch"""
        test_file = tmp_path / "type_mismatch.c"
        test_file.write_text("int main() { int x; char* p; x = p; return 0; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_semantic_error_duplicate_declaration(self, tmp_path):
        """Test detection of duplicate declarations"""
        test_file = tmp_path / "duplicate.c"
        test_file.write_text("int main() { int x; int x; return 0; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_semantic_error_undeclared_function(self, tmp_path):
        """Test detection of undeclared function"""
        test_file = tmp_path / "undeclared_func.c"
        test_file.write_text("int main() { foo(); return 0; }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
        assert 'error' in result.stderr.lower()
    
    def test_exit_code_success(self, tmp_path):
        """Test exit code 0 for successful compilation"""
        test_file = tmp_path / "success.c"
        test_file.write_text("int main() { return 0; }")
        output_file = tmp_path / "success"
        
        cmd = ['python3', '-m', 'toolchain.main', '-o', str(output_file), str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode == 0
    
    def test_exit_code_failure(self, tmp_path):
        """Test non-zero exit code for compilation errors"""
        test_file = tmp_path / "error.c"
        test_file.write_text("int main() { invalid syntax here }")
        
        cmd = ['python3', '-m', 'toolchain.main', str(test_file)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=COMPILER_ROOT)
        assert result.returncode != 0
