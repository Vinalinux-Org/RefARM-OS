# ============================================================
# Makefile for VinixOS Bootloader
# ============================================================

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE := $(CROSS_COMPILE)size

# Directories
SRC_DIR := src
INC_DIR := include
LINKER_DIR := linker
TOOLS_DIR := tools
BUILD_DIR := build

# Flags
CFLAGS := -mcpu=cortex-a8 \
          -marm \
          -mno-thumb-interwork \
          -nostdlib \
          -nostartfiles \
          -ffreestanding \
          -fno-builtin \
          -O2 \
          -Wall \
          -Wextra \
          -I$(INC_DIR)

ASFLAGS := -mcpu=cortex-a8 -marm

LDFLAGS := -T $(LINKER_DIR)/bootloader.ld \
           -nostdlib \
           -Map=$(BUILD_DIR)/bootloader.map

# Source files
ASM_SRCS := $(SRC_DIR)/arch/arm/start.S
C_SRCS := $(SRC_DIR)/main.c \
          $(SRC_DIR)/uart.c \
          $(SRC_DIR)/clock.c \
          $(SRC_DIR)/ddr.c \
          $(SRC_DIR)/mmc.c \
          $(SRC_DIR)/utils.c

# Object files
ASM_OBJS := $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SRCS:.S=.o)))
C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
OBJS := $(ASM_OBJS) $(C_OBJS)

# Targets
TARGET := bootloader
ELF := $(BUILD_DIR)/$(TARGET).elf
BIN := $(BUILD_DIR)/$(TARGET).bin
MLO := MLO

# ============================================================
# Build rules
# ============================================================

.PHONY: all clean size

all: $(MLO)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Assemble
$(BUILD_DIR)/%.o: $(SRC_DIR)/arch/arm/%.S | $(BUILD_DIR)
	@echo "AS  $<"
	@$(CC) $(ASFLAGS) -c $< -o $@

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Link
$(ELF): $(OBJS)
	@echo "LD  $@"
	@$(LD) $(LDFLAGS) $(OBJS) -o $@
	@$(OBJDUMP) -D $@ > $(BUILD_DIR)/$(TARGET).dis
	@echo ""
	@$(SIZE) $@

# Create binary
$(BIN): $(ELF)
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

# MLO: Add GP header
$(MLO): $(BIN)
	@echo "GEN MLO"
	@python3 $(TOOLS_DIR)/gp_header.py $< $@
	@echo ""
	@echo "Build complete!"
	@echo "Output: $(MLO)"
	@ls -lh $(MLO)

# Show size
size: $(ELF)
	@$(SIZE) $<

# Clean
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(MLO)
	@echo "Done."

# Help
help:
	@echo "VinixOS Bootloader Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build MLO (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  size    - Show binary size"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make           # Build bootloader"
	@echo "  make clean     # Clean build"
	@echo ""
	@echo "Output:"
	@echo "  MLO            # Bootable image for SD card"
	@echo "  build/         # Build directory (ELF, map, disassembly)"