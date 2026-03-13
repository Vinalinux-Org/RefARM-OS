# Self-hosted Compiler cho ARMv7-A

Cross-compiler tự phát triển chuyển đổi mã nguồn Subset C thành binary ELF ARMv7-A có thể thực thi trên VinixOS Platform (BeagleBone Black).

## Tính Năng

- **Ngôn ngữ nguồn**: Subset C (int, char, pointer, arrays, if/else, while, for, functions)
- **Target**: ARMv7-A (BeagleBone Black chạy VinixOS)
- **Implementation**: Python 3.8+
- **Pipeline**: Lexer → Parser → Semantic Analyzer → IR Generator → Code Generator → Assembler → Linker

## Cài Đặt

### Yêu Cầu Hệ Thống

- Python 3.8 hoặc mới hơn
- GNU ARM toolchain: `arm-linux-gnueabihf-gcc`, `arm-linux-gnueabihf-as`, `arm-linux-gnueabihf-ld`
- (Tùy chọn) QEMU ARM emulator: `qemu-arm`

### Cài Đặt Dependencies

```bash
# Install Python dependencies
pip install -r requirements.txt

# Install ARM toolchain (Ubuntu/Debian)
sudo apt-get install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# Install QEMU (optional, for testing)
sudo apt-get install qemu-user
```

## Sử Dụng

### Compile Chương Trình

```bash
# Compile source file to executable
python -m toolchain.main input.c -o output.elf

# Compile to assembly only
python -m toolchain.main input.c -S -o output.s

# Debug: dump tokens
python -m toolchain.main input.c --dump-tokens

# Debug: dump AST
python -m toolchain.main input.c --dump-ast

# Debug: dump IR
python -m toolchain.main input.c --dump-ir
```

### Chạy Tests

```bash
# Run all tests
make test

# Run unit tests only
make test-unit

# Run property tests only
make test-property

# Run integration tests
make test-integration
```

## Kiến Trúc

```
toolchain/
├── common/          # Shared utilities (error, config)
├── frontend/        # Frontend phases
│   ├── lexer/       # Lexical analysis
│   ├── parser/      # Syntax analysis
│   └── semantic/    # Semantic analysis
├── middleend/       # IR generation
│   └── ir/          # IR definitions
├── backend/         # Code generation
│   └── armv7a/      # ARMv7-A backend
└── utils/           # Utilities
```

## Tài Liệu

- [Architecture](docs/architecture.md) - Kiến trúc tổng thể
- [Subset C Specification](docs/subset_c_spec.md) - Ngôn ngữ Subset C
- [IR Format](docs/ir_format.md) - Intermediate Representation
- [Code Generation](docs/codegen_strategy.md) - Chiến lược sinh mã
- [Usage Guide](docs/usage_guide.md) - Hướng dẫn sử dụng

