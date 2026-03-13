# ==============================================================================
# VinixOS Master Makefile
# ==============================================================================

MAKEFLAGS += --no-print-directory

.PHONY: all bootloader userspace kernel clean

# Default target builds everything
all: bootloader userspace kernel
	@echo "========================================"
	@echo " VinixOS Build Complete                 "
	@echo "========================================"

bootloader:
	@if ! $(MAKE) -q -C bootloader; then \
		echo "========================================"; \
		echo " Building Bootloader                    "; \
		echo "========================================"; \
		$(MAKE) -C bootloader; \
	fi

userspace:
	@if ! $(MAKE) -q -C userspace; then \
		echo "========================================"; \
		echo " Building Userspace                     "; \
		echo "========================================"; \
		$(MAKE) -C userspace; \
	fi

kernel: userspace
	@if ! $(MAKE) -q -C kernel; then \
		echo "========================================"; \
		echo " Building Kernel                        "; \
		echo "========================================"; \
		$(MAKE) -C kernel; \
	fi

clean:
	@echo "========================================"
	@echo " Cleaning Bootloader                    "
	@$(MAKE) -C bootloader clean
	@echo " Cleaning Userspace                     "
	@$(MAKE) -C userspace clean
	@echo " Cleaning Kernel                        "
	@$(MAKE) -C kernel clean
	@echo "========================================"
	@echo " Clean Complete                         "
