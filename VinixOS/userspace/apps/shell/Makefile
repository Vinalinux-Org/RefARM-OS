# ==============================================================================
# VinixOS Userspace Application: shell
# ==============================================================================

include ../../rules.mk

APP_NAME = shell
APP_BUILD_DIR = $(BUILD_DIR)/apps/$(APP_NAME)
LIB_BUILD_DIR = $(BUILD_DIR)/lib

# Source files
SRC_C = $(wildcard *.c)
OBJ_C = $(addprefix $(APP_BUILD_DIR)/, $(SRC_C:.c=.o))

# Targets
TARGET_ELF = $(APP_BUILD_DIR)/$(APP_NAME).elf
TARGET_BIN = $(APP_BUILD_DIR)/$(APP_NAME).bin

# Required Library components
TARGET_CRT0 = $(LIB_BUILD_DIR)/crt0.o
TARGET_LIB  = $(LIB_BUILD_DIR)/librefarm.a

all: $(APP_BUILD_DIR) $(TARGET_ELF) $(TARGET_BIN)

$(APP_BUILD_DIR):
	@mkdir -p $(APP_BUILD_DIR)

$(APP_BUILD_DIR)/%.o: %.c | $(APP_BUILD_DIR)
	@echo "  CC      apps/$(APP_NAME)/$<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_ELF): $(TARGET_CRT0) $(TARGET_LIB) $(OBJ_C)
	@echo "  LD      $(TARGET_ELF)"
	@$(LD) $(LDFLAGS) $(TARGET_CRT0) $(OBJ_C) -L$(LIB_BUILD_DIR) -lrefarm $(LIBGCC) -o $@

$(TARGET_BIN): $(TARGET_ELF)
	@echo "  OBJCOPY $(TARGET_BIN)"
	@$(OBJCOPY) -O binary $< $@

clean:
	@rm -rf $(APP_BUILD_DIR)

.PHONY: all clean
