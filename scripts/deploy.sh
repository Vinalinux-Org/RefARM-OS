#!/bin/bash
set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 /dev/sdX"
    exit 1
fi

DEVICE=$1
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Determine partition names
if [[ "$DEVICE" == *"mmcblk"* ]]; then
    BOOT="${DEVICE}p1"
else
    BOOT="${DEVICE}1"
fi

[ ! -b "$BOOT" ] && echo "Error: $BOOT not found! Run setup_sdcard.sh first." && exit 1

echo "=== Checking bootloader ==="
if [ ! -f "$PROJECT_ROOT/bootloader/MLO" ]; then
    echo "Error: MLO not found!"
    echo "Run: cd scripts && ./download_bootloader.sh"
    exit 1
fi

echo "=== Building kernel ==="
cd "$PROJECT_ROOT/build"
make clean
make all

echo "=== Creating uImage ==="
if ! command -v mkimage &> /dev/null; then
    echo "Installing u-boot-tools..."
    sudo apt update && sudo apt install -y u-boot-tools
fi

mkimage -A arm -O linux -T kernel -C none \
    -a 0x80000000 -e 0x80000000 \
    -n "RefARM-OS" -d kernel.bin uImage

echo "=== Mounting SD card ==="
MOUNT="/tmp/bbb_boot"
sudo mkdir -p $MOUNT
sudo umount $MOUNT 2>/dev/null || true
sudo mount $BOOT $MOUNT

echo "=== Copying files ==="
sudo cp "$PROJECT_ROOT/bootloader/MLO" $MOUNT/
sudo cp "$PROJECT_ROOT/bootloader/u-boot.img" $MOUNT/
sudo cp "$PROJECT_ROOT/bootloader/uEnv.txt" $MOUNT/
sudo cp uImage $MOUNT/

echo "=== Files on SD card ==="
ls -lh $MOUNT

echo "=== Syncing ==="
sync
sudo umount $MOUNT

echo ""
echo "✓ Deploy complete!"
echo ""
echo "Next steps:"
echo "  1. Insert SD card into BeagleBone Black"
echo "  2. Connect UART (GND-1, RX-4, TX-5)"
echo "  3. Run: ./serial_console.sh"
echo "  4. Hold BOOT button and power on"