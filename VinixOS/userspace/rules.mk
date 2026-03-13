# ==============================================================================
# VinixOS Userspace Rules (rules.mk)
# ==============================================================================
# This file is included by all userspace component Makefiles.

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
AR      = $(CROSS_COMPILE)ar

# Directories
USERSPACE_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC_DIR        := $(USERSPACE_ROOT)/include
LIB_DIR        := $(USERSPACE_ROOT)/lib
BUILD_DIR      := $(USERSPACE_ROOT)/build

# Compiler Flags for ARMv7-A (Soft Float)
CFLAGS = -Wall -Wextra -O2 -g \
         -ffreestanding -nostdlib -fno-builtin \
         -march=armv7-a -mfloat-abi=soft \
         -I$(INC_DIR) \
         -I$(USERSPACE_ROOT)/libc/include

# Linker Flags
LDFLAGS = -nostdlib -T $(USERSPACE_ROOT)/linker/app.ld

# Compiler built-ins (required for soft-float division like __aeabi_uidivmod)
LIBGCC = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
