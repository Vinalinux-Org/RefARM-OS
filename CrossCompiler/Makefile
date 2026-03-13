# Phase 2 Compiler Makefile

.PHONY: all test test-unit test-property test-integration clean help

all: test

# Run all tests
test: test-unit test-property test-integration

# Run unit tests
test-unit:
	@echo "========================================="
	@echo " Running Unit Tests"
	@echo "========================================="
	pytest tests/unit/ -v

# Run property-based tests
test-property:
	@echo "========================================="
	@echo " Running Property-Based Tests"
	@echo "========================================="
	pytest tests/property/ -v --hypothesis-profile=ci

# Run integration tests
test-integration:
	@echo "========================================="
	@echo " Running Integration Tests"
	@echo "========================================="
	pytest tests/integration/ -v

# Run tests with coverage
test-cov:
	pytest tests/ --cov=toolchain --cov-report=html --cov-report=term

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf __pycache__ .pytest_cache .hypothesis .mypy_cache
	@rm -rf toolchain/__pycache__ toolchain/*/__pycache__ toolchain/*/*/__pycache__
	@rm -rf tests/__pycache__ tests/*/__pycache__
	@rm -rf htmlcov .coverage
	@rm -rf build/ dist/ *.egg-info
	@find . -type f -name '*.pyc' -delete
	@find . -type d -name '__pycache__' -delete
	@echo "Clean complete."

# Format code with black
format:
	black toolchain/ tests/

# Type check with mypy
typecheck:
	mypy toolchain/

# Help
help:
	@echo "Phase 2 Compiler Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make test              - Run all tests"
	@echo "  make test-unit         - Run unit tests only"
	@echo "  make test-property     - Run property-based tests only"
	@echo "  make test-integration  - Run integration tests only"
	@echo "  make test-cov          - Run tests with coverage report"
	@echo "  make clean             - Clean build artifacts"
	@echo "  make format            - Format code with black"
	@echo "  make typecheck         - Type check with mypy"
	@echo "  make help              - Show this help message"
