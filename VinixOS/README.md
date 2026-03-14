# VinixOS

Hệ điều hành tối giản cho ARMv7-A, chạy trên BeagleBone Black.

## Tổng Quan

VinixOS là bare-metal OS demonstrate các khái niệm cơ bản:
- Boot sequence và hardware initialization
- Memory management với MMU (3G/1G virtual memory split)
- Exception và interrupt handling
- Preemptive multitasking (round-robin scheduler)
- System call interface (AAPCS-compliant)
- Virtual filesystem với RAMFS
- User/kernel space isolation

**Target Hardware**: BeagleBone Black (TI AM335x, Cortex-A8)

## Tính Năng

- Boot chain hoàn chỉnh: ROM → MLO → Kernel
- MMU-based virtual memory (User: 0x40000000, Kernel: 0xC0000000)
- Exception handling (7 types)
- Preemptive scheduler (10ms time slice)
- System calls (write, read, open, close, exec, yield, ...)
- VFS + RAMFS
- Interactive shell (User mode)
- ELF binary loading
- UART console (115200 8N1)

## Cấu Trúc Project

```
VinixOS/
├── bootloader/         # MLO (first-stage bootloader)
├── kernel/             # VinixOS kernel
│   ├── src/
│   │   ├── arch/arm/   # ARM-specific (entry, vectors, context switch)
│   │   ├── drivers/    # UART, Timer, INTC
│   │   └── kernel/     # Core (scheduler, syscalls, VFS, MMU)
│   ├── include/        # Headers
│   └── linker/         # Linker scripts
├── userspace/          # User applications
│   ├── apps/shell/     # Interactive shell
│   ├── lib/            # Syscall wrappers, crt0
│   └── libc/           # Minimal C library
├── docs/               # Tài liệu kỹ thuật
└── scripts/            # Build scripts
```

## Yêu Cầu Hệ Thống

**OS**: Ubuntu 22.04 LTS (khuyến nghị)

**Hardware**:
- BeagleBone Black board
- microSD card (tối thiểu 128MB)
- USB-to-Serial adapter (3.3V TTL, 115200 8N1)

**Software**:
```bash
# Cài dependencies
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi
sudo apt-get install python3 python3-pip make screen

# Hoặc dùng script tự động (chạy từ thư mục gốc RefARM-OS/)
sudo bash scripts/setup-environment.sh
```

## Build

```bash
# Build toàn bộ (bootloader + kernel + userspace)
make

# Hoặc build riêng
make -C bootloader
make -C userspace
make -C kernel
```

## Deploy Lên SD Card

```bash
# Tự động (chạy từ trong VinixOS/)
make sdcard SDCARD=/dev/sdX

# Hoặc dùng script (chạy từ thư mục gốc RefARM-OS/)
sudo bash scripts/flash_sdcard.sh /dev/sdX
```

Script sẽ:
1. Copy MLO lên FAT partition
2. Ghi kernel.bin vào sector 2048 (offset 1MB)

## Chạy

1. Cắm SD card vào BeagleBone Black
2. Kết nối serial console:
   ```bash
   screen /dev/ttyUSB0 115200
   ```
3. Bật nguồn
4. Tương tác với shell

## Shell Commands

```
$ help       # Hiện commands
$ ls         # List files trong RAMFS
$ cat <file> # Hiện nội dung file
$ ps         # Hiện running tasks
$ meminfo    # Hiện memory info
$ exec <app> # Chạy ELF binary
```

## Memory Map

### Physical Memory
```
0x80000000 - 0x9FFFFFFF: DDR3 RAM (512MB)
0x44E00000 - 0x44E0FFFF: L4_WKUP Peripherals
0x48000000 - 0x482FFFFF: L4_PER Peripherals
```

### Virtual Memory
```
0x40000000 - 0x40FFFFFF: User Space (16MB)
0xC0000000 - 0xC7FFFFFF: Kernel Space (128MB)
0x44E00000 - 0x44E0FFFF: Peripherals (identity mapped)
0x48000000 - 0x482FFFFF: Peripherals (identity mapped)
```

## Kiến Trúc

### Boot Sequence

```
Power On → ROM Code → MLO → Kernel (entry.S) → kernel_main() → Scheduler
```

**MLO** khởi tạo:
- Clock configuration (DDR PLL @ 400MHz)
- DDR3 memory (512MB)
- MMC/SD interface
- Load kernel từ SD sector 2048

**Kernel entry**:
- Clear BSS
- Setup page table (1-level, section descriptors)
- Enable MMU (identity + high VA mapping)
- Reload stack pointers
- Trampoline sang high VA (0xC0000000)

### MMU Configuration

- 1-level page table: 4096 entries × 1MB sections
- User Space: VA 0x40000000 → PA 0x80000000 (AP=11, User RW)
- Kernel Space: VA 0xC0000000 → PA 0x80000000 (AP=01, Kernel-only)
- Peripherals: Identity mapped (Strongly Ordered)

### Task Scheduling

- Algorithm: Round-robin preemptive
- Time slice: 10ms (100 Hz timer)
- Tasks: Idle (kernel mode) + Shell (user mode)
- Context switch: Save/restore r0-r12, SP, LR, SPSR, SP_usr, LR_usr

### System Calls

**ABI**: AAPCS-compliant
- r7 = syscall number
- r0-r3 = arguments
- r0 = return value

**Syscalls**: write, read, exit, yield, open, read_file, close, listdir, get_tasks, get_meminfo, exec

### Filesystem

- VFS: Abstraction layer
- RAMFS: In-memory read-only filesystem
- Files embedded vào kernel image qua `.incbin`

## Tài Liệu

Tài liệu kỹ thuật chi tiết trong `docs/`:

1. `01-boot-and-bringup.md` - Boot sequence
2. `02-kernel-initialization.md` - Kernel startup
3. `03-memory-and-mmu.md` - Memory management
4. `04-interrupt-and-exception.md` - Exception handling
5. `05-task-and-scheduler.md` - Task scheduling
6. `06-syscall-mechanism.md` - Syscall interface
7. `07-filesystem-vfs-ramfs.md` - Filesystem
8. `08-userspace-application.md` - User applications
9. `99-system-overview.md` - Big picture

## Development

### Thêm Files Vào RAMFS

1. Đặt file trong `kernel/src/kernel/files/`
2. Rebuild kernel: `make -C kernel`
3. Files tự động embedded qua `payload.S`

### Tạo User Application Mới

1. Tạo app directory trong `userspace/apps/`
2. Viết app dùng syscall wrappers từ `userspace/lib/`
3. Link với `userspace/linker/app.ld`
4. Build: `make -C userspace`
5. Embed binary vào kernel hoặc load qua filesystem

## Debugging

**Serial Console**:
```bash
screen /dev/ttyUSB0 115200
# hoặc
minicom -D /dev/ttyUSB0 -b 115200
```

**Common Issues**:
- Boot hangs: Kiểm tra UART connection, verify MLO trên SD
- MMU fault: Kiểm tra page table, verify VA/PA mapping
- Task crash: Kiểm tra stack size, verify syscall arguments
- No shell prompt: Verify user payload loaded, check scheduler

**Debug Output**: Enable trace macros trong `kernel/include/trace.h`

## Testing

```bash
# Run integration tests
make test
```

## Nguyên Tắc Thiết Kế

1. Simplicity Over Features: Minimal implementation
2. Correctness Over Performance: Ưu tiên đúng đắn
3. Explicit Over Implicit: Behavior rõ ràng trong code
4. Isolation Over Sharing: User/kernel separation rõ ràng

## Giới Hạn

- Single-core only
- Static memory allocation
- Maximum 4 tasks
- Read-only filesystem
- No dynamic process creation
- No networking
- No power management

Các giới hạn này giữ codebase đơn giản và tập trung vào core OS concepts.
