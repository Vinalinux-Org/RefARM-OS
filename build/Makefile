# RefARM-OS Makefile

CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Directories
SRC = ../src
LINKER = ../linker

# Source files
ASM_SRC = $(SRC)/arch/arm/boot/reset.S $(SRC)/arch/arm/boot/entry.S
C_SRC = $(SRC)/kernel/main.c

# Objects
ASM_OBJ = reset.o entry.o
C_OBJ = main.o uart.o watchdog.o
OBJS = $(ASM_OBJ) $(C_OBJ)

# Flags
CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding \
         -march=armv7-a -mtune=cortex-a8 \
         -mfloat-abi=soft \
         -mno-unaligned-access \
         -fno-builtin \
         -fno-stack-protector \
         -I$(SRC)/include \
         -I$(SRC)/arch/arm/include \
         -O2 -g
ASFLAGS = -march=armv7-a -mfloat-abi=soft
LDFLAGS = -T $(LINKER)/kernel.ld -nostdlib -Wl,-Map=kernel.map

# Targets
all: kernel.bin

# Create output directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)/arch/arm/boot
	@mkdir -p $(OBJ_DIR)/arch/arm/lib
	@mkdir -p $(OBJ_DIR)/kernel

# Compile assembly files
reset.o: $(SRC)/arch/arm/boot/reset.S
	$(CC) $(ASFLAGS) -c $< -o $@

entry.o: $(SRC)/arch/arm/boot/entry.S
	$(CC) $(ASFLAGS) -c $< -o $@

main.o: $(SRC)/kernel/main.c
	$(CC) $(CFLAGS) -c $< -o $@

uart.o: $(SRC)/drivers/uart.c
	$(CC) $(CFLAGS) -c $< -o $@

watchdog.o: $(SRC)/drivers/watchdog.c
	$(CC) $(CFLAGS) -c $< -o $@


kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc
	@echo "=== Kernel sections ==="
	@$(CROSS_COMPILE)size $@

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo ""
	@echo "Build complete: kernel.bin"
	@ls -lh kernel.bin

clean:
	rm -f *.o *.elf *.bin *.map uImage

.PHONY: all clean