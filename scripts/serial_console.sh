#!/bin/bash

# Find serial device
DEVICE=""
for dev in /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyACM0; do
    [ -e "$dev" ] && DEVICE=$dev && break
done

if [ -z "$DEVICE" ]; then
    echo "Error: No serial device found!"
    echo "Available devices:"
    ls -l /dev/tty* | grep -E "USB|ACM"
    exit 1
fi

echo "Found: $DEVICE"
echo "Opening console (115200 8N1)..."
echo "Exit: Ctrl+A then K then Y"
echo ""

# Try to use without sudo first
if [ -r "$DEVICE" ] && [ -w "$DEVICE" ]; then
    screen $DEVICE 115200
else
    echo "No permission, using sudo..."
    echo "Tip: sudo usermod -a -G dialout \$USER (then logout/login)"
    sudo screen $DEVICE 115200
fi