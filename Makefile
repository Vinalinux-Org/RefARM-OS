# RefARM-OS Top-Level Makefile
# Build entire project: VinixOS + CrossCompiler

.PHONY: all clean vinixos compiler help test

# Default target
all: vinixos compiler
	@echo ""
	@echo "========================================="
	@echo "RefARM-OS Build Complete!"
	@echo "========================================="
	@echo "VinixOS kernel built successfully"
	@echo "VinCC compiler ready to use"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Install compiler: bash scripts/install_compiler.sh"
	@echo "  2. Deploy to SD card: bash scripts/flash_sdcard.sh /dev/sdX"
	@echo "  3. Connect serial: screen /dev/ttyUSB0 115200"
	@echo ""

# Build VinixOS (bootloader + kernel)
vinixos:
	@echo "========================================="
	@echo "Building VinixOS..."
	@echo "========================================="
	$(MAKE) -C VinixOS

# Build/verify compiler
compiler:
	@echo ""
	@echo "========================================="
	@echo "Verifying VinCC Compiler..."
	@echo "========================================="
	@if [ ! -f CrossCompiler/toolchain/main.py ]; then \
		echo "Error: Compiler source not found"; \
		exit 1; \
	fi
	@echo "Compiler source verified"
	@echo "Run 'bash scripts/install_compiler.sh' to install"

# Clean all build artifacts
clean:
	@echo "Cleaning VinixOS..."
	$(MAKE) -C VinixOS clean
	@echo "Cleaning CrossCompiler..."
	$(MAKE) -C CrossCompiler clean
	@echo "Clean complete"

# Run tests
test: vinixos
	@echo "Running VinixOS tests..."
	$(MAKE) -C VinixOS test
	@echo ""
	@echo "Running Compiler tests..."
	$(MAKE) -C CrossCompiler test

# Help target
help:
	@echo "RefARM-OS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build VinixOS and verify compiler (default)"
	@echo "  vinixos    - Build VinixOS only (bootloader + kernel)"
	@echo "  compiler   - Verify compiler source"
	@echo "  clean      - Clean all build artifacts"
	@echo "  test       - Run test suites"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Quick Start:"
	@echo "  1. make all                              # Build everything"
	@echo "  2. bash scripts/install_compiler.sh      # Install compiler"
	@echo "  3. bash scripts/flash_sdcard.sh /dev/sdX # Deploy to SD"
	@echo ""
	@echo "For detailed instructions, see README.md"
