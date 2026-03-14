# VinixOS Makefile

CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Directories
SRC = src
LINKER = linker
# Directory containing the userspace payload (shell)
USERSPACE_DIR = ../userspace
BUILD_DIR = build
INITFS_DIR = ../initfs

# Source files
ASM_BOOT_SRC = $(SRC)/arch/arm/entry/reset.S $(SRC)/arch/arm/entry/entry.S
ASM_EXC_SRC = $(SRC)/arch/arm/exceptions/vectors.S $(SRC)/arch/arm/exceptions/exception_entry.S
C_KERNEL_SRC = $(SRC)/kernel/main.c $(SRC)/kernel/exceptions/exception_handlers.c $(SRC)/kernel/irq/irq_core.c \
               $(SRC)/kernel/core/svc_handler.c
C_DRIVER_SRC = $(SRC)/drivers/uart.c $(SRC)/drivers/watchdog.c $(SRC)/drivers/intc.c $(SRC)/drivers/timer.c
C_FS_SRC = $(SRC)/kernel/fs/vfs.c $(SRC)/kernel/fs/ramfs.c
C_LIBC_SRC = libc/src/string.c

# RAMFS build-time filesystem - auto-discover files in initfs/
INITFS_FILES = $(shell find $(INITFS_DIR) -type f 2>/dev/null)
RAMFS_GENERATED_H = $(BUILD_DIR)/ramfs_generated.h
RAMFS_GENERATED_C = $(BUILD_DIR)/ramfs_generated.c
RAMFS_GENERATED_S = $(BUILD_DIR)/ramfs_payload.S

# Objects (Flattened in BUILD_DIR)
ASM_OBJ = $(addprefix $(BUILD_DIR)/, vectors.o reset.o entry.o exception_entry.o context_switch.o mmu_enable.o)
C_OBJ = $(addprefix $(BUILD_DIR)/, main.o exception_handlers.o irq_core.o uart.o watchdog.o intc.o timer.o scheduler.o task.o idle.o svc_handler.o mmu.o payload.o vfs.o ramfs.o ramfs_generated.o string.o)
OBJS = $(ASM_OBJ) $(C_OBJ)

# Flags
CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding \
         -march=armv7-a -mtune=cortex-a8 \
         -mfloat-abi=soft \
         -mno-unaligned-access \
         -fno-builtin \
         -fno-stack-protector \
         -Iinclude \
         -Ilibc/include \
         -I$(SRC)/arch/arm/include \
         -I$(USERSPACE_DIR)/include \
         -O2 -g
ASFLAGS = -march=armv7-a -mfloat-abi=soft
LDFLAGS = -T $(LINKER)/kernel.ld -nostdlib -Wl,-Map=$(BUILD_DIR)/kernel.map

# Targets
all: $(BUILD_DIR)/kernel.bin

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Userspace Dependencies - Build shell
PAYLOAD_BIN = $(USERSPACE_DIR)/build/apps/shell/shell.bin
$(PAYLOAD_BIN):
	$(MAKE) -C $(USERSPACE_DIR)

# Payload Embed Target
$(BUILD_DIR)/payload.o: $(SRC)/kernel/payload.S $(PAYLOAD_BIN) $(RAMFS_GENERATED_S) | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@


# Assembly files - Boot
$(BUILD_DIR)/reset.o: $(SRC)/arch/arm/entry/reset.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/entry.o: $(SRC)/arch/arm/entry/entry.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# Assembly files - Exceptions
$(BUILD_DIR)/vectors.o: $(SRC)/arch/arm/exceptions/vectors.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/exception_entry.o: $(SRC)/arch/arm/exceptions/exception_entry.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# Assembly files - Scheduler
$(BUILD_DIR)/context_switch.o: $(SRC)/arch/arm/scheduler/context_switch.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# C files - Kernel
$(BUILD_DIR)/main.o: $(SRC)/kernel/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/exception_handlers.o: $(SRC)/kernel/exceptions/exception_handlers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/irq_core.o: $(SRC)/kernel/irq/irq_core.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/scheduler.o: $(SRC)/kernel/scheduler/scheduler.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/task.o: $(SRC)/kernel/scheduler/task.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idle.o: $(SRC)/kernel/scheduler/idle.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/svc_handler.o: $(SRC)/kernel/core/svc_handler.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# C files - Drivers
$(BUILD_DIR)/uart.o: $(SRC)/drivers/uart.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/watchdog.o: $(SRC)/drivers/watchdog.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/intc.o: $(SRC)/drivers/intc.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/timer.o: $(SRC)/drivers/timer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assembly files - MMU
$(BUILD_DIR)/mmu_enable.o: $(SRC)/arch/arm/mmu/mmu_enable.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# C files - MMU
$(BUILD_DIR)/mmu.o: $(SRC)/kernel/mmu/mmu.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# C files - Filesystem
$(BUILD_DIR)/vfs.o: $(SRC)/kernel/fs/vfs.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ramfs.o: $(SRC)/kernel/fs/ramfs.c $(RAMFS_GENERATED_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# RAMFS build-time filesystem generation
$(RAMFS_GENERATED_H) $(RAMFS_GENERATED_C) $(RAMFS_GENERATED_S): $(INITFS_FILES) ../scripts/generate_ramfs_table.py | $(BUILD_DIR)
	@echo "  GEN     ramfs_generated.c/h/S"
	@python3 ../scripts/generate_ramfs_table.py \
		--source-dir $(INITFS_DIR) \
		--output-header $(RAMFS_GENERATED_H) \
		--output-source $(RAMFS_GENERATED_C) \
		--output-assembly $(RAMFS_GENERATED_S)

$(BUILD_DIR)/ramfs_generated.o: $(RAMFS_GENERATED_C) $(RAMFS_GENERATED_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(RAMFS_GENERATED_C) -o $@

# C files - libc
$(BUILD_DIR)/string.o: libc/src/string.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# Link
$(BUILD_DIR)/kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc
	@echo "=== Kernel sections ==="
	@$(CROSS_COMPILE)size $@

# Create binary
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo ""
	@echo "Build complete: $@"
	@ls -lh $@

clean:
	@echo "  CLEAN   kernel headers, objects and elf"
	@rm -rf $(BUILD_DIR) *.o *.elf *.bin *.map uImage
	@echo "  CLEAN   userspace payload"
	@$(MAKE) -C $(USERSPACE_DIR) clean

.PHONY: all clean