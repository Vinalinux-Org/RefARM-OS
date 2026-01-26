#!/bin/bash
set -e

# ============================================================
# deploy.sh - Deploy kernel to SD card
# ============================================================

if [ $# -ne 1 ]; then
    echo "Usage: $0 /dev/sdX"
    echo ""
    echo "Example: $0 /dev/sdb"
    echo ""
    echo "Available disks:"
    lsblk -d -o NAME,SIZE,TYPE,TRAN | grep -E "usb|mmc" || echo "  (none)"
    exit 1
fi

DEVICE=$1

# Determine partition name
if [[ "$DEVICE" == *"mmcblk"* ]]; then
    BOOT_PART="${DEVICE}p1"
else
    BOOT_PART="${DEVICE}1"
fi

# Check if partition exists
if [ ! -b "$BOOT_PART" ]; then
    echo "Error: Boot partition $BOOT_PART not found!"
    echo "Run setup_sdcard.sh first."
    exit 1
fi

# Project paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BOOTLOADER_DIR="$PROJECT_ROOT/bootloader"

echo "========================================"
echo " Deploy RefARM-OS to SD card"
echo "========================================"
echo "Device: $DEVICE"
echo "Boot partition: $BOOT_PART"
echo ""

# Check if kernel exists
if [ ! -f "$BUILD_DIR/kernel.bin" ]; then
    echo "Error: kernel.bin not found!"
    echo "Run 'make' in build/ directory first."
    exit 1
fi

# Show kernel info
echo "Kernel binary:"
ls -lh "$BUILD_DIR/kernel.bin"
echo ""

# Mount
MOUNT_POINT="/tmp/sdcard_boot_$$"
mkdir -p $MOUNT_POINT

echo "Mounting $BOOT_PART..."
sudo mount $BOOT_PART $MOUNT_POINT

# Backup old kernel (if exists)
if [ -f "$MOUNT_POINT/kernel.bin" ]; then
    echo "Backing up old kernel.bin..."
    sudo mv "$MOUNT_POINT/kernel.bin" "$MOUNT_POINT/kernel.bin.old"
fi

# Copy files
echo "Copying files..."
sudo cp -v "$BUILD_DIR/kernel.bin" $MOUNT_POINT/
sudo cp -v "$BOOTLOADER_DIR/uEnv.txt" $MOUNT_POINT/

# Optional: copy bootloader if present
if [ -f "$BOOTLOADER_DIR/MLO" ]; then
    echo "Copying bootloader..."
    sudo cp -v "$BOOTLOADER_DIR/MLO" $MOUNT_POINT/
    sudo cp -v "$BOOTLOADER_DIR/u-boot.img" $MOUNT_POINT/
fi

# Verify
echo ""
echo "Files on SD card:"
ls -lh $MOUNT_POINT/ | grep -E "kernel|MLO|u-boot|uEnv"

echo ""
echo "uEnv.txt content:"
cat $MOUNT_POINT/uEnv.txt

# Sync and unmount
echo ""
echo "Syncing..."
sync
sudo umount $MOUNT_POINT
rmdir $MOUNT_POINT

echo ""
echo "========================================"
echo " Deploy complete!"
echo "========================================"
echo ""
echo "Next steps:"
echo "  1. Insert SD card into BeagleBone Black"
echo "  2. Connect UART (J1 header, 115200 8N1)"
echo "  3. Run: ./scripts/serial_console.sh"
echo "  4. Power on BBB"
echo ""