# ==============================================================================
# RefARM-OS Userspace Master Makefile
# ==============================================================================

# Default app to build if not specified (e.g., make APP=hello_world)
APP ?= shell

.PHONY: all lib app clean help

all: lib app

# 1. Build the userspace static library (librefarm.a and crt0.o)
lib:
	@echo "========================================"
	@echo " Building Userspace Library (librefarm) "
	@echo "========================================"
	@$(MAKE) -C lib

# 2. Build the specific application
app: lib
	@echo "========================================"
	@echo " Building Application: $(APP)           "
	@echo "========================================"
	@$(MAKE) -C apps/$(APP) APP=$(APP)

clean:
	@echo "========================================"
	@echo " Cleaning Userspace Build Artifacts     "
	@echo "========================================"
	@rm -rf build/
	@echo "  Clean complete."

help:
	@echo "RefARM-OS Userspace Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make APP=<app_name>    # Build specific app"
	@echo "  make                   # Build default app (shell)"
	@echo "  make clean             # Clean all build artifacts"
	@echo ""
	@echo "Available apps:"
	@echo "  shell                  # Interactive shell (default)"
