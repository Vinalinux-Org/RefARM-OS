#!/bin/bash
# RefARM-OS Environment Setup Script
# Ubuntu 22.04 LTS
#
# USAGE: Chạy từ thư mục gốc của project với quyền root:
#   sudo bash scripts/setup-environment.sh

set -e

echo "Installing dependencies for RefARM-OS..."

# Update package lists
apt-get update
apt-get upgrade -y

# Build tools
apt-get install build-essential -y
apt-get install make -y
apt-get install gcc -y
apt-get install g++ -y
apt-get install flex -y
apt-get install bison -y
apt-get install git -y

# Libraries (cần để build cross-compiler toolchain từ source nếu cần)
apt-get install libgmp3-dev -y
apt-get install libmpc-dev -y
apt-get install libmpfr-dev -y
apt-get install libisl-dev -y
apt-get install texinfo -y

# ARM toolchain
apt-get install gcc-arm-none-eabi -y
apt-get install binutils-arm-none-eabi -y
apt-get install binutils-arm-linux-gnueabihf -y

# Python
apt-get install python3 -y
apt-get install python3-pip -y
apt-get install python3-venv -y

# Serial console
apt-get install screen -y
apt-get install minicom -y

# Multilib support
apt-get install gcc-multilib -y
apt-get install libc6-i386 -y

# SD card flashing tools
apt-get install parted -y
apt-get install dosfstools -y       # mkfs.vfat cho FAT32 partition

# Install Python development dependencies cho VinCC compiler
echo ""
echo "Installing Python compiler dependencies..."
if [ -f "CrossCompiler/requirements.txt" ]; then
    pip3 install -r CrossCompiler/requirements.txt
else
    echo "Warning: CrossCompiler/requirements.txt not found."
    echo "Make sure to run this script from the project root directory."
fi

echo ""
echo "Installation complete!"
echo ""
echo "Next steps:"
echo "  cd VinixOS && make"
echo "  bash scripts/install_compiler.sh"
echo ""
