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
ASM_BOOT_SRC = $(SRC)/arch/arm/boot/reset.S $(SRC)/arch/arm/boot/entry.S
ASM_EXC_SRC = $(SRC)/arch/arm/exceptions/vectors.S $(SRC)/arch/arm/exceptions/exception_entry.S
C_KERNEL_SRC = $(SRC)/kernel/main.c $(SRC)/kernel/exceptions/exception_handlers.c $(SRC)/kernel/irq/irq_core.c \
               $(SRC)/kernel/shell/shell.c $(SRC)/kernel/shell/commands.c
C_DRIVER_SRC = $(SRC)/drivers/uart.c $(SRC)/drivers/watchdog.c $(SRC)/drivers/intc.c $(SRC)/drivers/timer.c

# Objects
ASM_OBJ = vectors.o reset.o entry.o exception_entry.o context_switch.o
C_OBJ = main.o exception_handlers.o irq_core.o uart.o watchdog.o intc.o timer.o shell.o commands.o scheduler.o task.o idle.o
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

# Assembly files - Boot
reset.o: $(SRC)/arch/arm/boot/reset.S
	$(CC) $(ASFLAGS) -c $< -o $@

entry.o: $(SRC)/arch/arm/boot/entry.S
	$(CC) $(ASFLAGS) -c $< -o $@

# Assembly files - Exceptions
vectors.o: $(SRC)/arch/arm/exceptions/vectors.S
	$(CC) $(ASFLAGS) -c $< -o $@

exception_entry.o: $(SRC)/arch/arm/exceptions/exception_entry.S
	$(CC) $(ASFLAGS) -c $< -o $@

# Assembly files - Scheduler
context_switch.o: $(SRC)/arch/arm/scheduler/context_switch.S
	$(CC) $(ASFLAGS) -c $< -o $@

# C files - Kernel
main.o: $(SRC)/kernel/main.c
	$(CC) $(CFLAGS) -c $< -o $@

exception_handlers.o: $(SRC)/kernel/exceptions/exception_handlers.c
	$(CC) $(CFLAGS) -c $< -o $@

irq_core.o: $(SRC)/kernel/irq/irq_core.c
	$(CC) $(CFLAGS) -c $< -o $@

shell.o: $(SRC)/kernel/shell/shell.c
	$(CC) $(CFLAGS) -c $< -o $@

commands.o: $(SRC)/kernel/shell/commands.c
	$(CC) $(CFLAGS) -c $< -o $@

scheduler.o: $(SRC)/kernel/scheduler/scheduler.c
	$(CC) $(CFLAGS) -c $< -o $@

task.o: $(SRC)/kernel/scheduler/task.c
	$(CC) $(CFLAGS) -c $< -o $@

idle.o: $(SRC)/kernel/scheduler/idle.c
	$(CC) $(CFLAGS) -c $< -o $@

# C files - Drivers
uart.o: $(SRC)/drivers/uart.c
	$(CC) $(CFLAGS) -c $< -o $@

watchdog.o: $(SRC)/drivers/watchdog.c
	$(CC) $(CFLAGS) -c $< -o $@

intc.o: $(SRC)/drivers/intc.c
	$(CC) $(CFLAGS) -c $< -o $@

timer.o: $(SRC)/drivers/timer.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link
kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc
	@echo "=== Kernel sections ==="
	@$(CROSS_COMPILE)size $@

# Create binary
kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo ""
	@echo "Build complete: kernel.bin"
	@ls -lh kernel.bin

clean:
	rm -f *.o *.elf *.bin *.map uImage

.PHONY: all clean