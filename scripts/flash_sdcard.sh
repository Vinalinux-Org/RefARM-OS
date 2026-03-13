#!/bin/bash
# Update MLO on SD Card Boot Partition (FAT)
# Usage: ./flash_sdcard.sh /dev/sdX

DEVICE=$1

if [ -z "$DEVICE" ]; then
    echo "Usage: $0 <device>"
    echo "Example: $0 /dev/sdb"
    exit 1
fi

# Safety check: verify it's a valid block device
if ! sudo blockdev --getsz "$DEVICE" >/dev/null 2>&1; then
    echo "Error: Device $DEVICE is not a valid block device or requires root permissions."
    exit 1
fi

# Resolve Project Root (Robust)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOPDIR="$(dirname "$SCRIPT_DIR")"
MLO="$TOPDIR/VinixOS/bootloader/MLO"
KERNEL="$TOPDIR/VinixOS/kernel/build/kernel.bin"

echo "========================================"
echo " Update Bootloader (MLO)"
echo "========================================"
echo "Project Root: $TOPDIR"
echo "Device:       $DEVICE"

# Check MLO
if [ ! -f "$MLO" ]; then
    echo "Error: MLO not found at $MLO"
    echo "Please build bootloader first:"
    echo "  cd $TOPDIR/VinixOS/bootloader && make"
    exit 1
fi

# Check Kernel
if [ ! -f "$KERNEL" ]; then
    echo "Error: kernel.bin not found at $KERNEL"
    echo "Please build kernel first:"
    echo "  cd $TOPDIR/VinixOS/kernel && make"
    exit 1
fi

# Define Partition
BOOT_PART="${DEVICE}1"

# Check Partition
if [ ! -b "$BOOT_PART" ]; then
    echo "Error: Partition $BOOT_PART not found."
    exit 1
fi

# Mount Point Logic
MOUNTPOINT=$(lsblk -no MOUNTPOINT "$BOOT_PART" | head -n 1)
MY_MOUNT=0

if [ -z "$MOUNTPOINT" ]; then
    echo "Mounting $BOOT_PART..."
    MOUNTPOINT="/tmp/sd_boot_update_$$"
    mkdir -p "$MOUNTPOINT"
    sudo mount "$BOOT_PART" "$MOUNTPOINT"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to mount $BOOT_PART"
        rmdir "$MOUNTPOINT"
        exit 1
    fi
    MY_MOUNT=1
else
    echo "Using existing mount: $MOUNTPOINT"
fi

# Copy MLO (to FAT partition)
echo "Removing old MLO to prevent FAT fragmentation..."
sudo rm -f "$MOUNTPOINT/MLO"
sudo sync

echo "Copying MLO to FAT ($MOUNTPOINT)..."
sudo cp "$MLO" "$MOUNTPOINT/MLO"
sudo sync

# Flash Kernel (raw copy to device at sector 2048)
echo "Writing Kernel to $DEVICE at sector 2048 (1MB offset)..."
if ! sudo dd if="$KERNEL" of="$DEVICE" bs=512 seek=2048 conv=notrunc status=progress; then
    echo "Error: Failed to write Kernel via dd."
    if [ $MY_MOUNT -eq 1 ]; then
        sudo umount "$MOUNTPOINT"
        rmdir "$MOUNTPOINT"
    fi
    exit 1
fi
sudo sync

# Verify FAT copy
if [ -f "$MOUNTPOINT/MLO" ]; then
    echo "Success: MLO and Kernel (raw sector 2048) updated."
else
    echo "Error: MLO Copy failed."
fi

# Cleanup
if [ $MY_MOUNT -eq 1 ]; then
    echo "Unmounting..."
    sudo umount "$MOUNTPOINT"
    rmdir "$MOUNTPOINT"
fi

echo "Done."
