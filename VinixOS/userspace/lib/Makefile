# ==============================================================================
# VinixOS Userspace Library (librefarm.a)
# ==============================================================================

include ../rules.mk

# Output targets
LIB_BUILD_DIR = $(BUILD_DIR)/lib
TARGET_LIB    = $(LIB_BUILD_DIR)/librefarm.a
TARGET_CRT0   = $(LIB_BUILD_DIR)/crt0.o

# Source files
SRC_C = syscall.c
SRC_S = crt0.S
LIBC_SRC = ../libc/src/string.c

# Object files
OBJ_C = $(addprefix $(LIB_BUILD_DIR)/, $(SRC_C:.c=.o))
OBJ_S = $(addprefix $(LIB_BUILD_DIR)/, $(SRC_S:.S=.o))
OBJ_LIBC = $(LIB_BUILD_DIR)/string.o
OBJS  = $(OBJ_C) $(OBJ_S) $(OBJ_LIBC)

all: $(TARGET_CRT0) $(TARGET_LIB)

$(LIB_BUILD_DIR):
	@mkdir -p $(LIB_BUILD_DIR)

# crt0 must be kept separate because the linker needs it as the ENTRY object
$(TARGET_CRT0): $(SRC_S) | $(LIB_BUILD_DIR)
	@echo "  CC      lib/$(SRC_S)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(LIB_BUILD_DIR)/%.o: %.c | $(LIB_BUILD_DIR)
	@echo "  CC      lib/$<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_LIBC): $(LIBC_SRC) | $(LIB_BUILD_DIR)
	@echo "  CC      libc/src/string.c"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(OBJ_C) $(OBJ_LIBC)
	@echo "  AR      $(TARGET_LIB)"
	@$(AR) rcs $@ $^

clean:
	@rm -rf $(LIB_BUILD_DIR)

.PHONY: all clean
