#!/bin/bash
# Install script for vincc compiler

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_COMPILER_DIR="$ROOT_DIR/CrossCompiler/toolchain"

if [[ ! -d "$SRC_COMPILER_DIR" ]]; then
    echo -e "${RED}Error: compiler source directory not found at $SRC_COMPILER_DIR${NC}"
    exit 1
fi

run_local() {
    cd "$ROOT_DIR/CrossCompiler"
    exec python3 -m toolchain.main "$@"
}

MODE="${1:-install}"
if [[ "$MODE" == "run" || "$MODE" == "--run" ]]; then
    shift
    run_local "$@"
fi

echo -e "${GREEN}Installing vincc (VinAI C Compiler)...${NC}"

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    INSTALL_DIR="/usr/local/bin"
    COMPILER_DIR="/usr/local/lib/vincc"
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$COMPILER_DIR"
else
    INSTALL_DIR="$HOME/.local/bin"
    COMPILER_DIR="$HOME/.local/lib/vincc"

    # Create directories if they don't exist
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$HOME/.local/lib"
fi

echo "Installing to: $INSTALL_DIR"

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 is required but not installed${NC}"
    exit 1
fi

if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
    echo -e "${YELLOW}Warning: ARM toolchain not found${NC}"
    echo "Install with: sudo apt-get install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf"
fi

# Copy compiler files
echo -e "${YELLOW}Installing compiler files...${NC}"
rm -rf "$COMPILER_DIR/toolchain"
cp -a "$SRC_COMPILER_DIR" "$COMPILER_DIR/"

# Create wrapper script
cat > "$INSTALL_DIR/vincc" << EOF
#!/bin/bash
# vincc - VinAI C Compiler

# Get the directory where this script is located
COMPILER_DIR="$COMPILER_DIR"

# Check if compiler package directory exists
if [ ! -d "\$COMPILER_DIR/toolchain" ]; then
    echo "Error: Compiler package not found at \$COMPILER_DIR/toolchain"
    exit 1
fi

# Run the compiler
cd "\$COMPILER_DIR" && python3 -m toolchain.main "\$@"
EOF

chmod +x "$INSTALL_DIR/vincc"

# Add to PATH if needed
if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
    echo -e "${YELLOW}Adding $INSTALL_DIR to PATH...${NC}"

    if [[ $EUID -ne 0 ]]; then
        # User install - add to .bashrc
        if ! grep -Fq "export PATH=\"\$PATH:$INSTALL_DIR\"" ~/.bashrc 2>/dev/null; then
            echo "export PATH=\"\$PATH:$INSTALL_DIR\"" >> ~/.bashrc
            echo -e "${GREEN}Added to ~/.bashrc. Run 'source ~/.bashrc' or restart terminal.${NC}"
        else
            echo -e "${GREEN}PATH already configured in ~/.bashrc${NC}"
        fi
    else
        echo -e "${GREEN}$INSTALL_DIR should already be in system PATH${NC}"
    fi
fi

echo -e "${GREEN}Installation complete!${NC}"
echo
echo "Test installation:"
echo "  vincc --version"
echo "  vincc hello.c -o hello"