#!/usr/bin/env python3
"""
gp_header.py
Generate GP (General Purpose) header for AM335x ROM code

GP Header Format (32 bytes):
  Offset 0x00: Image size (4 bytes, little-endian)
  Offset 0x04: Load address (4 bytes, little-endian)
  Offset 0x08-0x1F: Reserved (zeros)

The ROM code reads this header to know:
  - How many bytes to load
  - Where to load them in SRAM
"""

import struct
import sys
import os

def make_gp_header(input_bin, output_mlo):
    """
    Generate MLO file with GP header for AM335x BootROM (GP device, non-XIP memory boot)

    TRM Table 26-37: GP Device Image Format
      Offset 0x00: Size        (4 bytes LE) = size of executable code in bytes
      Offset 0x04: Destination (4 bytes LE) = SRAM load address AND entry point
      Offset 0x08+: Executable code (binary)

    NOTE from TRM sec 26.1.10.2:
      "The Destination address field stands for both:
       - Target address for image copy from non-XIP storage to target location
       - Entry point for image code"
      → _start must be the FIRST instruction at LOAD_ADDR.

    NOTE on header size: The GP header is exactly 8 bytes (Size + Destination).
    The Size field = size of the EXECUTABLE CODE only (does NOT include the 8-byte header).
    BootROM reads Size bytes starting from offset 0x08 in the file and copies them
    to Destination. Execution then starts at Destination.

    Args:
        input_bin: Path to bootloader.bin (raw binary, no header)
        output_mlo: Path to output MLO file (8-byte header + binary)
    """
    # Load address in SRAM (OCMC RAM start per AM335x BootROM)
    # BootROM loads GP image to 0x402F0400 (Public RAM area, TRM sec 26.1.4.2)
    LOAD_ADDR = 0x402F0400

    # Read bootloader binary
    with open(input_bin, 'rb') as f:
        bootloader = f.read()

    # code_size = number of bytes of executable code (NOT including the 8-byte header)
    # TRM Table 26-37: Size field = "Size of the image" = executable code bytes only
    code_size = len(bootloader)

    # Create GP header (exactly 8 bytes per TRM Table 26-37)
    header = struct.pack('<I', code_size)   # Offset 0x00: Size of executable code
    header += struct.pack('<I', LOAD_ADDR)  # Offset 0x04: Destination = load addr = entry point

    # Write MLO file = 8-byte header + binary code
    with open(output_mlo, 'wb') as f:
        f.write(header)
        f.write(bootloader)

    # Print summary
    total_mlo_size = len(header) + code_size
    print(f"Generated {output_mlo}:")
    print(f"  Bootloader size: {code_size} bytes")
    print(f"  Total MLO size:  {total_mlo_size} bytes")
    print(f"  Load address:    0x{LOAD_ADDR:08X}")

    # Check size limit: BootROM OCMC RAM (public) = 111616 bytes (109 KB)
    # TRM sec 26.1.9.2: "maximum size of downloaded image is 109 KB"
    MAX_SIZE = 109 * 1024  # 111616 bytes
    if code_size > MAX_SIZE:
        print(f"  WARNING: Image size ({code_size} bytes) exceeds BootROM SRAM limit ({MAX_SIZE} bytes)!")
        return 1
    else:
        print(f"  SRAM usage:      {code_size}/{MAX_SIZE} bytes ({code_size*100//MAX_SIZE}%)")

    return 0

def main():
    if len(sys.argv) != 3:
        print("Usage: gp_header.py <input.bin> <output.MLO>")
        print("")
        print("Example:")
        print("  python3 gp_header.py bootloader.bin MLO")
        return 1
    
    input_bin = sys.argv[1]
    output_mlo = sys.argv[2]
    
    # Check input file exists
    if not os.path.exists(input_bin):
        print(f"Error: Input file '{input_bin}' not found")
        return 1
    
    return make_gp_header(input_bin, output_mlo)

if __name__ == '__main__':
    sys.exit(main())