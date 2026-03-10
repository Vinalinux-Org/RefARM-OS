# ==============================================================================
# RefARM-OS Userspace Master Makefile
# ==============================================================================

MAKEFLAGS += --no-print-directory

# Default app to build if not specified (e.g., make APP=hello_world)
APP ?= shell

.PHONY: all lib app clean help

all:
	@if $(MAKE) -q -C lib && $(MAKE) -q -C apps/$(APP) APP=$(APP); then \
		echo "make: Nothing to be done for 'all'."; \
	else \
		$(MAKE) build_lib build_app; \
	fi

build_lib:
	@if ! $(MAKE) -q -C lib; then \
		echo "========================================"; \
		echo " Building Userspace Library (librefarm) "; \
		echo "========================================"; \
		$(MAKE) -C lib; \
	fi

build_app: build_lib
	@if ! $(MAKE) -q -C apps/$(APP) APP=$(APP); then \
		echo "========================================"; \
		echo " Building Application: $(APP)           "; \
		echo "========================================"; \
		$(MAKE) -C apps/$(APP) APP=$(APP); \
	fi

lib: build_lib
app: build_app


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
