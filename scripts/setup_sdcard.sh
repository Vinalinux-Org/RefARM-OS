#!/bin/bash
set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 /dev/sdX"
    echo ""
    echo "Available disks:"
    lsblk -d -o NAME,SIZE,TYPE | grep disk
    exit 1
fi

DEVICE=$1

echo "WARNING: All data on $DEVICE will be DELETED!"
lsblk $DEVICE
read -p "Continue? (yes/no): " confirm
[ "$confirm" != "yes" ] && exit 1

echo "Unmounting..."
sudo umount ${DEVICE}* 2>/dev/null || true

echo "Creating partition table..."
sudo dd if=/dev/zero of=$DEVICE bs=1M count=10 status=none

sudo fdisk $DEVICE << EOF
o
n
p
1

+100M
t
c
a
n
p
2


w
EOF

sleep 2
sudo partprobe $DEVICE

# Determine partition names
if [[ "$DEVICE" == *"mmcblk"* ]]; then
    BOOT="${DEVICE}p1"
    ROOT="${DEVICE}p2"
else
    BOOT="${DEVICE}1"
    ROOT="${DEVICE}2"
fi

echo "Formatting..."
sudo mkfs.vfat -F 32 -n BOOT $BOOT
sudo mkfs.ext4 -L rootfs $ROOT

echo ""
echo "SD card ready!"
echo "  Boot: $BOOT (FAT32)"
echo "  Root: $ROOT (ext4)"