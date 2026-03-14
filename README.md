# RefARM-OS: Reference ARM Software Platform

Platform phần mềm hoàn chỉnh cho ARMv7-A, bao gồm hệ điều hành và compiler chạy trên hardware thật (BeagleBone Black).

## Giới Thiệu

RefARM-OS là một reference platform để học và hiểu cách xây dựng hệ thống phần mềm hoàn chỉnh từ đầu, chạy trên ARM hardware thật. Project bao gồm 2 components chính đã hoàn thành:

### Phase 1: VinixOS - Hệ Điều Hành

VinixOS là một bare-metal operating system tối giản nhưng đầy đủ chức năng, bao gồm:

**Bootloader (MLO)**:
- Khởi tạo hardware (clocks, DDR3 memory, peripherals)
- Load kernel từ SD card vào memory
- Jump sang kernel với boot parameters

**Kernel**:
- Memory Management Unit (MMU): Virtual memory với 3G/1G split (User space: 0x40000000, Kernel space: 0xC0000000)
- Exception Handling: 7 loại exceptions (Reset, Undefined, SVC, Prefetch Abort, Data Abort, IRQ, FIQ)
- Task Scheduler: Preemptive round-robin scheduler với 10ms time slice
- System Calls: 11 syscalls tuân thủ AAPCS calling convention
- Virtual File System (VFS): Abstraction layer cho filesystem operations
- RAMFS: In-memory read-only filesystem với files embedded trong kernel image
- Interrupt Controller: INTC configuration và IRQ routing

**Userspace**:
- Interactive Shell: Chạy ở User mode (privilege level thấp hơn kernel)
- Runtime Library: Minimal C library với syscall wrappers
- ELF Loader: Load và execute ELF binaries từ filesystem

**Mục đích**: Cung cấp platform thực tế để hiểu OS-hardware interaction và làm target để chạy programs được compile bởi VinCC compiler.

### Phase 2: VinCC - Cross Compiler

VinCC (Vinix C Compiler) là một compiler hoàn chỉnh viết bằng Python, compile Subset C sang ARMv7-A binary:

**Frontend**:
- Preprocessor: Xử lý `#include`, `#define` (basic)
- Lexer: Tokenization với line/column tracking
- Parser: Recursive descent parser sinh Abstract Syntax Tree (AST)
- Semantic Analyzer: Type checking, symbol table management, scope resolution

**Middle-end**:
- IR Generator: Sinh 3-address code intermediate representation
- Optimizer: Constant folding, constant propagation, dead code elimination

**Backend**:
- Code Generator: Sinh ARMv7-A assembly code
- Register Allocator: Linear scan register allocation
- AAPCS Compliance: Tuân thủ ARM Procedure Call Standard
- Assembler: Tích hợp với GNU assembler (arm-linux-gnueabihf-as)
- Linker: Tạo ELF32 executable với custom linker script

**Runtime Library (reflibc)**:
- Startup Code (crt0.S): Entry point `_start` → `main()` → `exit()`
- Syscall Wrappers: Interface với VinixOS syscalls
- I/O Functions: `print_str()`, `print_int()`, `print_hex()`, etc.
- Math Support: Software division (`__aeabi_idiv`)

**Ngôn ngữ hỗ trợ (Subset C)**:
- Data types: `int`, `char`, pointers, arrays
- Control flow: `if/else`, `while`, `for`, `return`, `break`, `continue`
- Functions: Definitions, calls, recursion (up to 4 parameters via r0-r3)
- Operators: Arithmetic, comparison, logical, bitwise, assignment

**Mục đích**: Hiểu toàn bộ compiler pipeline từ source code đến executable binary, và validate output trên real hardware thay vì emulation.

## Tại Sao Project Này Quan Trọng?

1. **Real Hardware Validation**: Tất cả code chạy trên BeagleBone Black thật, không phải emulator. Điều này force bạn hiểu đúng cách hardware hoạt động.

2. **Complete Stack**: Từ power-on button đến running compiled program, bạn hiểu mọi layer: bootloader → kernel → syscalls → compiler → runtime → application.

3. **No Black Boxes**: Không dùng Linux, GCC, hoặc bất kỳ existing OS/compiler nào. Mọi thứ được implement từ đầu nên bạn hiểu rõ từng detail.

4. **Educational Focus**: Code được viết để dễ hiểu, có documentation đầy đủ, không optimize quá mức làm mất tính rõ ràng.

5. **Practical Skills**: Học được skills thực tế: ARM assembly, hardware programming, OS concepts, compiler design, system integration.

## Yêu Cầu Hệ Thống

### Phần Cứng

**Bắt buộc**:
- **BeagleBone Black**: Board ARM Cortex-A8 (~$60)
  - SoC: Texas Instruments AM335x (1GHz Cortex-A8, ARMv7-A)
  - RAM: 512MB DDR3
  - Storage: microSD card slot
- **microSD Card**: Tối thiểu 128MB (khuyến nghị 4GB+)
- **USB-to-Serial Adapter**: 3.3V TTL level (FTDI hoặc tương đương)
  - Kết nối: GND, RX, TX
  - Baudrate: 115200 8N1
- **5V Power Supply**: 2A minimum (hoặc USB power)

**Optional**:
- SD card reader (để flash image từ PC)
- Breadboard và jumper wires (nếu muốn thêm peripherals)

### Phần Mềm

**Operating System**: Ubuntu 22.04 LTS (khuyến nghị mạnh)

Lý do chọn Ubuntu 22.04:
- Packages ổn định và được test kỹ
- ARM toolchain có sẵn trong apt repository
- Python 3.10 default (đủ cho compiler)
- Long-term support đến 2027

**Dependencies**:
- `gcc-arm-none-eabi`: ARM bare-metal cross-compiler (cho VinixOS)
- `binutils-arm-none-eabi`: ARM binutils (objcopy, objdump)
- `binutils-arm-linux-gnueabihf`: ARM assembler/linker (cho VinCC)
- `python3`: Python 3.8+ (cho VinCC compiler)
- `python3-pip`: Python package manager
- `build-essential`: GCC, make, và build tools
- `git`: Version control
- `screen` hoặc `minicom`: Serial console

## Cài Đặt và Setup

### Bước 1: Clone Repository

```bash
git clone git@github.com:Vinalinux-Org/Vinix-OS.git
cd Vinix-OS
```

### Bước 2: Cài Đặt Dependencies

**Tự động (khuyến nghị)**:

```bash
# Cấp quyền execute cho script
chmod +x scripts/setup-environment.sh

# Chạy script (cần quyền root để apt-get install)
sudo bash scripts/setup-environment.sh
```

Script sẽ tự động cài:
- ARM cross-compiler và binutils
- Python 3 và pip
- Build tools (make, gcc)
- Serial console tools (screen, minicom)

**Thủ công** (nếu muốn control từng bước):

```bash
# Update package lists
sudo apt-get update

# ARM cross-compiler (cho VinixOS)
sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi

# ARM binutils (cho VinCC compiler)
sudo apt-get install -y binutils-arm-linux-gnueabihf

# Python 3
sudo apt-get install -y python3 python3-pip

# Build tools
sudo apt-get install -y build-essential make git

# Serial console
sudo apt-get install -y screen minicom
```

### Bước 3: Build VinixOS

```bash
cd VinixOS
make
```

Build process sẽ:
1. Compile bootloader (MLO) từ `bootloader/`
2. Compile kernel từ `kernel/`
3. Compile userspace (shell) từ `userspace/`
4. Embed shell binary vào kernel image
5. Tạo `kernel.bin` ready để flash

Output files:
- `bootloader/build/MLO`: First-stage bootloader
- `kernel/build/kernel.bin`: Kernel image với embedded userspace

### Bước 4: Cài Đặt VinCC Compiler

```bash
cd ../CrossCompiler

# Cấp quyền execute
chmod +x ../scripts/install_compiler.sh

# Cài đặt
../scripts/install_compiler.sh
```

Script sẽ:
1. Copy toolchain vào `~/.local/lib/vincc/`
2. Tạo wrapper script `~/.local/bin/vincc`
3. Thêm `~/.local/bin` vào PATH (nếu chưa có)

Verify installation:
```bash
vincc --version
# Output: vincc 0.1.0
```

### Bước 5: Compile Test Program

```bash
# Compile hello world
vincc tests/programs/test_hello.c -o test_hello

# Verify output
file test_hello
# Output: test_hello: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV)
```

### Bước 6: Embed Program Vào Kernel

```bash
# Copy binary vào RAMFS
cp test_hello ../VinixOS/kernel/src/kernel/files/

# Rebuild kernel để embed binary
cd ../VinixOS/kernel
make
```

### Bước 7: Flash Lên SD Card

```bash
cd ../..

# Cấp quyền execute
chmod +x scripts/flash_sdcard.sh

# Flash (thay sdX bằng device của bạn, ví dụ: sdb)
sudo ./scripts/flash_sdcard.sh /dev/sdX
```

**Cảnh báo**: Lệnh này sẽ ghi đè toàn bộ SD card. Đảm bảo chọn đúng device!

Để tìm device của SD card:
```bash
# Trước khi cắm SD card
lsblk

# Sau khi cắm SD card
lsblk
# Device mới xuất hiện là SD card của bạn (thường là /dev/sdb hoặc /dev/mmcblk0)
```

Script sẽ:
1. Tạo FAT32 partition đầu tiên
2. Copy MLO vào FAT partition
3. Ghi kernel.bin raw vào sector 2048 (offset 1MB)

### Bước 8: Kết Nối Serial Console

```bash
# Sử dụng screen
screen /dev/ttyUSB0 115200

# Hoặc minicom
minicom -D /dev/ttyUSB0 -b 115200
```

**Lưu ý**: Bạn có thể cần thêm user vào group `dialout`:
```bash
sudo usermod -a -G dialout $USER
# Logout và login lại để apply
```

### Bước 9: Boot BeagleBone Black

1. Cắm SD card vào BeagleBone Black
2. Giữ nút BOOT (gần SD card slot)
3. Cắm nguồn 5V (hoặc USB)
4. Thả nút BOOT sau 2-3 giây

Board sẽ boot từ SD card và bạn sẽ thấy boot sequence trên serial console:

```
========================================
VinixOS Bootloader
========================================
Board:  BeagleBone Black (AM335x)
UART:   Initialized @ 115200 8N1
Clock:  ROM default (48 MHz UART)

Clock:  Configuring DDR PLL for 400MHz... Done.
DDR:    Initializing 256MB DDR3...
DDR:    Running memory test...
DDR:    Test PASSED
MMC:    Initializing SD card...
MMC:    Loading kernel from SD card...
========================================
Boot:   Jumping to kernel @ 0x80000000
========================================

VinixOS: Interactive Shell
========================================

[BOOT] Initializing Virtual File System...
[BOOT] Loading User App Payload to 0x40000000
[BOOT] Starting Scheduler...

VinixOS Shell
Type 'help' for commands

$ _
```

### Bước 10: Sử Dụng Shell

```bash
$ help
Available commands:
  help     - Show this help
  ls       - List files
  cat      - Display file content
  ps       - Show processes
  meminfo  - Show memory info
  exec     - Execute program

$ ls
test_hello  hello.txt  info.txt

$ exec test_hello
Hello, VinixOS!
42

$ _
```

## Workflow Hoàn Chỉnh

### 1. Viết Chương Trình C

`fibonacci.c`:
```c
#include "reflibc.h"

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i;
    for (i = 0; i < 10; i = i + 1) {
        print_int(fib(i));
        print_str("\n");
    }
    return 0;
}
```

### 2. Compile Với VinCC

```bash
vincc fibonacci.c -o fibonacci
```

### 3. Nhúng Vào VinixOS

```bash
cp fibonacci VinixOS/kernel/src/kernel/files/
cd VinixOS/kernel
make
```

### 4. Deploy Lên SD Card

```bash
cd ../..
bash scripts/flash_sdcard.sh /dev/sdX
```

### 5. Chạy Trên BeagleBone Black

```
VinixOS Shell
$ ls
fibonacci  hello.txt  info.txt

$ exec fibonacci
0
1
1
2
3
5
8
13
21
34

$ _
```

## Cấu Trúc Project

```
RefARM-OS/
├── VinixOS/              # Hệ điều hành
│   ├── bootloader/       # MLO bootloader
│   ├── kernel/           # VinixOS kernel
│   ├── userspace/        # User applications
│   └── docs/             # Tài liệu kỹ thuật OS
│
├── CrossCompiler/        # Compiler toolchain
│   ├── toolchain/        # Compiler implementation
│   │   ├── frontend/     # Lexer, Parser, Semantic
│   │   ├── middleend/    # IR generation
│   │   ├── backend/      # ARMv7-A code generator
│   │   └── runtime/      # reflibc runtime library
│   ├── tests/            # Test suite
│   └── docs/             # Tài liệu kỹ thuật compiler
│
├── scripts/              # Build và deployment scripts
│   ├── setup-environment.sh    # Cài dependencies
│   ├── install_compiler.sh     # Cài VinCC
│   └── flash_sdcard.sh         # Deploy lên SD card
│
├── PROJECT_SCOPE.md      # Project scope chi tiết
├── README.md             # File này
└── Makefile              # Top-level build
```

## Tính Năng

### VinixOS

- Boot chain hoàn chỉnh (ROM → MLO → Kernel)
- MMU-based virtual memory (3G/1G split)
- Exception và interrupt handling (7 types)
- Preemptive multitasking (round-robin scheduler)
- System call interface (11 syscalls)
- Virtual filesystem với RAMFS
- Interactive shell (User mode)
- ELF binary loading và execution
- Tài liệu kỹ thuật đầy đủ

### VinCC Compiler

- Complete compiler pipeline (Lexer → Parser → Semantic → IR → Codegen)
- Subset C support (int, char, pointer, array, control flow)
- ARMv7-A code generation (AAPCS calling convention)
- Integrated runtime library (reflibc)
- VinixOS syscall integration
- ELF32 binary output
- Optimizations (constant folding, dead code elimination)
- Test suite đầy đủ

### Ngôn Ngữ Hỗ Trợ (Subset C)

**Data Types**: int, char, pointers, arrays  
**Control Flow**: if/else, while, for, return, break, continue  
**Functions**: Definitions, calls, recursion  
**Operators**: Arithmetic, comparison, logical, bitwise, assignment

## Tài Liệu

### VinixOS Documentation

Tài liệu kỹ thuật trong `VinixOS/docs/`:
- Boot sequence và hardware initialization
- Memory management và MMU configuration
- Interrupt handling và exception flow
- Task scheduling và context switching
- System call mechanism
- Filesystem implementation
- Userspace application development

Xem [VinixOS/README.md](VinixOS/README.md) để biết thêm chi tiết.

### Compiler Documentation

Tài liệu kỹ thuật trong `CrossCompiler/docs/`:
- Compiler architecture và pipeline
- Subset C language specification
- IR format và optimizations
- Code generation strategy
- Usage guide và examples

Xem [CrossCompiler/README.md](CrossCompiler/README.md) để biết thêm chi tiết.

## Project Status

| Component | Status | Documentation |
|-----------|--------|---------------|
| Bootloader (MLO) | Hoàn thành | VinixOS/docs/01-boot-and-bringup.md |
| Kernel Core | Hoàn thành | VinixOS/docs/02-kernel-initialization.md |
| Memory Management | Hoàn thành | VinixOS/docs/03-memory-and-mmu.md |
| Interrupt Handling | Hoàn thành | VinixOS/docs/04-interrupt-and-exception.md |
| Task Scheduler | Hoàn thành | VinixOS/docs/05-task-and-scheduler.md |
| System Calls | Hoàn thành | VinixOS/docs/06-syscall-mechanism.md |
| Filesystem | Hoàn thành | VinixOS/docs/07-filesystem-vfs-ramfs.md |
| Userspace Runtime | Hoàn thành | VinixOS/docs/08-userspace-application.md |
| Shell Application | Hoàn thành | VinixOS/docs/08-userspace-application.md |
| Compiler Frontend | Hoàn thành | CrossCompiler/docs/architecture.md |
| Compiler Middle-end | Hoàn thành | CrossCompiler/docs/ir_format.md |
| Compiler Backend | Hoàn thành | CrossCompiler/docs/codegen_strategy.md |
| Runtime Library | Hoàn thành | CrossCompiler/docs/usage_guide.md |

## Mục Tiêu Project

### Mục Tiêu Giáo Dục

1. Hiểu OS-Hardware interaction trên hardware thật
2. Nắm vững ARM architecture qua hands-on implementation
3. Xây dựng compiler hoàn chỉnh từ đầu
4. Thực hành ABI và calling conventions
5. Tích hợp toàn bộ stack (bootloader, OS, compiler, applications)

### Mục Tiêu Kỹ Thuật

1. Reference platform được document đầy đủ
2. Validation trên hardware thật (không phải emulation)
3. Complete stack từ power-on đến running compiled programs
4. Reproducible - bất kỳ ai cũng có thể rebuild từ docs

### Không Phải Mục Tiêu

- Production-ready OS
- POSIX compliance
- High performance
- Feature completeness
- Commercial use

## Nguyên Tắc Thiết Kế

1. **Simplicity First**: Minimal implementation để demonstrate concepts
2. **Explicit Over Implicit**: Behavior rõ ràng trong code
3. **Correctness Over Performance**: Ưu tiên implementation đúng
4. **Real Hardware Focus**: Không có emulation shortcuts

## Troubleshooting

### Common Issues

**Boot fails**:
- Verify SD card format đúng
- Check MLO ở đúng location (sector 0)
- Verify serial connection (115200 8N1)

**MMU faults**:
- Check page table configuration
- Verify VA/PA mappings
- Enable debug output trong `trace.h`

**Task crashes**:
- Check stack size
- Verify syscall arguments
- Check stack overflow (canary)

**Compiler errors**:
- Verify feature được support trong Subset C
- Check `CrossCompiler/docs/subset_c_spec.md`

## FAQ

**Tại sao build OS và compiler từ đầu?**  
Để hiểu sâu cách OS và compiler hoạt động qua implementation thực tế, không chỉ đọc lý thuyết.

**Tại sao không dùng Linux hoặc RTOS có sẵn?**  
Đây là educational project. Dùng OS có sẵn sẽ che giấu implementation details cần học.

**Tại sao BeagleBone Black?**  
Giá cả phải chăng (~$60), hardware được document tốt, không cần proprietary tools, real ARM hardware.

**Tại sao ARMv7-A thay vì ARMv8?**  
ARMv7-A đơn giản hơn để hiểu nhưng vẫn đủ modern. Balance tốt giữa simplicity và relevance.

**Có thể dùng cho production không?**  
Không. Đây là educational/reference project, thiếu nhiều features cần cho production.

**Có cần mua hardware không?**  
Cần BeagleBone Black để chạy VinixOS. Tuy nhiên có thể học code và docs mà không cần hardware.

**Compiler có thể compile chính nó không?**  
Chưa. Self-hosting là future enhancement. Hiện tại compiler viết bằng Python, generate ARM code.

**Compile được những program nào?**  
Bất kỳ program nào dùng Subset C features. Xem `CrossCompiler/docs/subset_c_spec.md`.

## Roadmap

### Phase 1: VinixOS Platform (Hoàn thành)

- Bootloader với hardware initialization
- Kernel với MMU, scheduler, interrupts
- System call interface
- Virtual filesystem
- User space runtime
- Interactive shell
- ELF binary loading
- Tài liệu đầy đủ

### Phase 2: VinCC Cross-Compiler (Hoàn thành)

- Lexer và tokenization
- Parser và AST generation
- Semantic analysis
- IR generation (3-address code)
- Basic optimizations
- ARMv7-A code generator
- Register allocation
- AAPCS calling convention
- Assembler và linker integration
- VinixOS syscall emission
- End-to-end testing trên hardware
- Runtime library (reflibc)
- Tài liệu đầy đủ

### Future Enhancements (Optional)

- Self-hosting capability (compiler chạy trên VinixOS)
- Additional language features (struct, union, enum)
- Advanced optimizations
- Multi-core support (SMP)
- Dynamic memory allocation
- Writable filesystem (FAT32)
- Network stack
- Debug symbol generation (DWARF)

## References

### Hardware Documentation

- [AM335x Technical Reference Manual](https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf)
- [BeagleBone Black System Reference Manual](https://github.com/beagleboard/beaglebone-black/wiki/System-Reference-Manual)
- [Cortex-A8 Technical Reference Manual](https://developer.arm.com/documentation/ddi0344/latest/)

### ARM Architecture

- [ARM Architecture Reference Manual ARMv7-A](https://developer.arm.com/documentation/ddi0406/latest/)
- [ARM Procedure Call Standard (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst)

### OS Development

- [OSDev Wiki](https://wiki.osdev.org/)
- [xv6: A simple Unix-like teaching OS](https://pdos.csail.mit.edu/6.828/2020/xv6.html)

### Compiler Development

- [Engineering a Compiler (Cooper & Torczon)](https://www.elsevier.com/books/engineering-a-compiler/cooper/978-0-12-088478-0)
- [Modern Compiler Implementation in C (Appel)](https://www.cs.princeton.edu/~appel/modern/c/)


## Acknowledgments

Project này được xây dựng như learning platform để hiểu:
- Operating system fundamentals trên real hardware
- ARM architecture và assembly programming
- Compiler design và implementation từ đầu
- System-level programming và hardware interaction
- Complete software stack integration

Project demonstrate rằng complex systems có thể được hiểu bằng cách build chúng từ first principles.

---

**Project Status**: Cả Phase 1 (VinixOS) và Phase 2 (VinCC Compiler) đã hoàn thành và test trên real hardware.

**Lưu ý**: Đây là educational/reference project demonstrate OS và compiler concepts trên real hardware. Không dành cho production use.

## Cấu Trúc Project Chi Tiết

```
RefARM-OS/
│
├── VinixOS/                    # Hệ điều hành
│   ├── bootloader/             # MLO first-stage bootloader
│   │   ├── src/                # Bootloader source
│   │   │   ├── main.c          # Boot flow (8 stages)
│   │   │   ├── clock.c         # Clock configuration
│   │   │   ├── ddr.c           # DDR3 initialization
│   │   │   ├── mmc.c           # SD card driver
│   │   │   └── uart.c          # UART driver
│   │   ├── include/            # Hardware definitions
│   │   └── linker/             # Linker script (SRAM)
│   │
│   ├── kernel/                 # VinixOS kernel
│   │   ├── src/
│   │   │   ├── arch/arm/       # ARM-specific code
│   │   │   │   └── entry/
│   │   │   │       ├── entry.S         # Boot entry, MMU enable
│   │   │   │       ├── vectors.S       # Exception vectors
│   │   │   │       └── context_switch.S # Task switching
│   │   │   ├── drivers/        # Hardware drivers
│   │   │   │   ├── uart.c      # UART driver
│   │   │   │   ├── timer.c     # DMTimer2 (scheduler tick)
│   │   │   │   ├── intc.c      # Interrupt controller
│   │   │   │   └── watchdog.c  # Watchdog timer
│   │   │   └── kernel/         # Core kernel
│   │   │       ├── main.c              # Kernel entry point
│   │   │       ├── core/
│   │   │       │   └── svc_handler.c   # Syscall dispatcher
│   │   │       ├── scheduler/
│   │   │       │   ├── scheduler.c     # Round-robin scheduler
│   │   │       │   ├── task.c          # Task management
│   │   │       │   └── idle.c          # Idle task
│   │   │       ├── mmu/
│   │   │       │   └── mmu.c           # MMU configuration
│   │   │       ├── irq/
│   │   │       │   └── irq_core.c      # IRQ framework
│   │   │       ├── fs/
│   │   │       │   ├── vfs.c           # Virtual filesystem
│   │   │       │   └── ramfs.c         # RAMFS implementation
│   │   │       ├── files/              # Files to embed
│   │   │       └── payload.S           # File embedding
│   │   ├── include/            # Kernel headers
│   │   └── linker/             # Linker script (DDR, high VA)
│   │
│   ├── userspace/              # User applications
│   │   ├── apps/shell/         # Interactive shell
│   │   ├── lib/
│   │   │   ├── crt0.S          # Startup code
│   │   │   └── syscall.c       # Syscall wrappers
│   │   ├── libc/               # Minimal C library
│   │   └── linker/             # Linker script (User space)
│   │
│   └── docs/                   # Tài liệu kỹ thuật OS
│       ├── 01-boot-and-bringup.md
│       ├── 02-kernel-initialization.md
│       ├── 03-memory-and-mmu.md
│       ├── 04-interrupt-and-exception.md
│       ├── 05-task-and-scheduler.md
│       ├── 06-syscall-mechanism.md
│       ├── 07-filesystem-vfs-ramfs.md
│       ├── 08-userspace-application.md
│       └── 99-system-overview.md
│
├── CrossCompiler/              # Compiler toolchain
│   ├── toolchain/              # Compiler source
│   │   ├── main.py             # Entry point
│   │   ├── common/
│   │   │   ├── config.py       # Compiler configuration
│   │   │   └── error.py        # Error handling
│   │   ├── frontend/
│   │   │   ├── preprocessor/   # #include, #define
│   │   │   ├── lexer/          # Tokenization
│   │   │   ├── parser/         # AST generation
│   │   │   └── semantic/       # Type checking
│   │   ├── middleend/
│   │   │   └── ir/             # IR generation, optimization
│   │   ├── backend/
│   │   │   └── armv7a/         # Code generator
│   │   │       ├── code_generator.py
│   │   │       ├── register_allocator.py
│   │   │       ├── assembler.py
│   │   │       └── linker.py
│   │   └── runtime/            # reflibc
│   │       ├── reflibc.h       # Runtime header
│   │       ├── reflibc.c       # I/O implementations
│   │       ├── syscalls.S      # Syscall wrappers
│   │       ├── crt0.S          # Startup code
│   │       ├── divmod.S        # Division support
│   │       └── app.ld          # Linker script
│   │
│   ├── tests/                  # Test suite
│   │   ├── test_integration.py
│   │   └── programs/           # Test programs
│   │
│   └── docs/                   # Tài liệu kỹ thuật compiler
│       ├── architecture.md
│       ├── subset_c_spec.md
│       ├── ir_format.md
│       ├── codegen_strategy.md
│       └── usage_guide.md
│
├── scripts/                    # Build và deployment scripts
│   ├── setup-environment.sh    # Cài dependencies
│   ├── install_compiler.sh     # Cài VinCC
│   └── flash_sdcard.sh         # Deploy lên SD card
│
├── PROJECT_SCOPE.md            # Project scope chi tiết
├── README.md                   # File này
└── Makefile                    # Top-level build
```

## Kiến Trúc Hệ Thống

### Memory Map

**Physical Memory**:
```
0x00000000 - 0x001FFFFF: Boot ROM (2MB)
0x402F0000 - 0x4030FFFF: Internal SRAM (128KB) - MLO loads here
0x44E00000 - 0x44E0FFFF: L4_WKUP Peripherals (UART0, CM_PER, WDT)
0x48000000 - 0x482FFFFF: L4_PER Peripherals (INTC, Timer, GPIO)
0x4C000000 - 0x4C000FFF: EMIF (DDR Controller)
0x80000000 - 0x9FFFFFFF: DDR3 RAM (512MB)
```

**Virtual Memory** (sau khi MMU enable):
```
0x00000000 - 0x3FFFFFFF: Unmapped (Translation Fault nếu access)

0x40000000 - 0x40FFFFFF: User Space (16MB)
                         → PA 0x80000000
                         AP=11 (User RW, Kernel RW)
                         Cached (Write-back)
                         
0x41000000 - 0xBFFFFFFF: Unmapped

0xC0000000 - 0xC7FFFFFF: Kernel Space (128MB)
                         → PA 0x80000000
                         AP=01 (Kernel-only)
                         Cached (Write-back)
                         
0xC8000000 - 0x44DFFFFF: Unmapped

0x44E00000 - 0x44E0FFFF: L4_WKUP Peripherals
                         → PA 0x44E00000 (Identity mapped)
                         AP=01 (Kernel-only)
                         Strongly Ordered (No cache, no reorder)
                         
0x44E10000 - 0x47FFFFFF: Unmapped

0x48000000 - 0x482FFFFF: L4_PER Peripherals
                         → PA 0x48000000 (Identity mapped)
                         AP=01 (Kernel-only)
                         Strongly Ordered
                         
0x48300000 - 0xFFFFFFFF: Unmapped
```

**Giải thích**:
- User và Kernel share cùng physical memory (PA 0x80000000) nhưng có separate virtual address ranges
- MMU enforce access permissions: User code không thể access Kernel VA (Permission Fault)
- Peripherals identity mapped (VA == PA) để đơn giản
- Strongly Ordered cho peripherals: Mỗi access phải hit hardware, không cache, không reorder

### Boot Sequence

```
Power On
    ↓
ROM Code (built-in SoC)
    - Đọc boot configuration pins
    - Load MLO từ SD card vào SRAM (0x402F0400)
    - Verify GP header (size, load address, checksum)
    - Jump to MLO entry point
    ↓
MLO Bootloader (chạy trong SRAM)
    Stage 0: Enable essential clocks (L4LS, L3, L4FW)
    Stage 1: Disable watchdog timer
    Stage 2: Initialize UART (115200 8N1)
    Stage 3: Print boot banner
    Stage 4: Configure DDR PLL (400MHz)
    Stage 5: Initialize DDR3 memory (512MB @ 0x80000000)
    Stage 6: Initialize MMC, load kernel từ sector 2048
    Stage 7: Setup boot parameters
    Stage 8: Jump to kernel @ 0x80000000
    ↓
Kernel Entry (entry.S, chạy tại PA, MMU OFF)
    - Clear BSS section
    - Build L1 page table (1-level, section descriptors)
    - Enable MMU (identity + high VA mapping)
    - Reload stack pointers sang VA
    - Trampoline: Jump sang kernel_main() @ VA 0xC0xxxxxx
    ↓
kernel_main() (chạy tại VA, MMU ON)
    - mmu_init(): Remove identity mapping, update VBAR
    - Initialize INTC, IRQ framework
    - Initialize timer (10ms tick)
    - Initialize VFS, mount RAMFS
    - Load user payload (shell) vào User Space
    - Initialize scheduler
    - Add Idle task (kernel mode)
    - Add Shell task (user mode)
    - Enable IRQ at CPU level
    - scheduler_start() → NEVER RETURNS
    ↓
Scheduler Running
    - Timer interrupt every 10ms
    - Round-robin giữa Idle và Shell
    - Shell chạy ở User mode, accept commands
    - User có thể exec compiled programs
```

### System Call Flow

```
User Application (User Mode, 0x40000000)
    ↓ Call syscall wrapper
Syscall Wrapper (user code)
    - Setup registers: r7=syscall_number, r0-r3=arguments
    - Execute: svc #0
    ↓ SVC exception
CPU (Hardware)
    - Save CPSR → SPSR_svc
    - Switch to SVC mode
    - Disable IRQ (set I-bit)
    - Save return address → LR_svc
    - Set PC = VBAR + 0x08 (SVC vector)
    ↓
svc_handler_entry (Assembly, kernel code)
    - Save context: r0-r12, LR, SPSR to SVC stack
    - Call svc_handler(context_pointer)
    ↓
svc_handler() (C code, kernel)
    - Read syscall number từ context->r7
    - Dispatch to appropriate handler (switch statement)
    - Handler validates arguments, performs operation
    - Write return value vào context->r0
    - Return
    ↓
svc_handler_entry (Assembly)
    - Restore context từ stack
    - Execute: rfeia sp! (restore SPSR → CPSR, return)
    ↓
CPU (Hardware)
    - Restore CPSR từ SPSR_svc (switch back to User mode)
    - Resume execution at LR_svc
    ↓
Syscall Wrapper
    - Return (r0 contains return value)
    ↓
User Application
    - Continue execution
```

## Tài Liệu Chi Tiết

### VinixOS Documentation

Tài liệu kỹ thuật đầy đủ trong `VinixOS/docs/`, mỗi file giải thích một subsystem:

1. **01-boot-and-bringup.md**: Boot sequence từ power-on đến kernel_main()
   - ROM boot process
   - MLO 8 stages chi tiết
   - Kernel entry và MMU trampoline
   - Memory map evolution

2. **02-kernel-initialization.md**: Kernel startup và subsystem initialization
   - Hardware re-initialization
   - MMU Phase B (remove identity mapping)
   - INTC và IRQ setup
   - VFS và RAMFS mount
   - Scheduler initialization

3. **03-memory-and-mmu.md**: Memory management và MMU configuration
   - ARMv7-A MMU basics
   - Page table structure (1-level, section descriptors)
   - Memory regions chi tiết
   - Address translation flow
   - Design decisions và rationale

4. **04-interrupt-and-exception.md**: Exception handling và IRQ flow
   - ARMv7-A exception model
   - Vector table layout
   - Exception handlers (Undefined, Abort, SVC, IRQ)
   - INTC configuration
   - IRQ routing và nested interrupts

5. **05-task-and-scheduler.md**: Task management và scheduling
   - Task structure (PCB)
   - Context switch mechanism
   - Round-robin scheduling algorithm
   - Timer configuration
   - Stack management và canary

6. **06-syscall-mechanism.md**: System call interface
   - AAPCS calling convention
   - Syscall ABI (r7=number, r0-r3=args)
   - SVC handler implementation
   - Syscall implementations chi tiết
   - Pointer validation

7. **07-filesystem-vfs-ramfs.md**: Filesystem implementation
   - VFS abstraction layer
   - RAMFS implementation
   - File embedding qua payload.S
   - File descriptor management
   - Shell commands (ls, cat)

8. **08-userspace-application.md**: User application development
   - Memory layout (0x40000000)
   - Runtime startup (crt0.S)
   - Linker script
   - Syscall wrappers
   - Build process
   - Privilege levels

9. **99-system-overview.md**: Big picture
   - Complete system architecture
   - Data flows
   - Component interactions
   - Design principles
   - Key achievements

Xem [VinixOS/README.md](VinixOS/README.md) để biết thêm chi tiết về OS.

### Compiler Documentation

Tài liệu kỹ thuật trong `CrossCompiler/docs/`:

1. **architecture.md**: Compiler architecture tổng thể
   - Pipeline overview
   - Component interactions
   - Data flow qua các phases

2. **subset_c_spec.md**: Ngôn ngữ Subset C specification
   - Supported features chi tiết
   - Syntax và semantics
   - Limitations và rationale

3. **ir_format.md**: Intermediate representation format
   - 3-address code structure
   - IR instructions
   - Optimization opportunities

4. **codegen_strategy.md**: Code generation strategy
   - ARMv7-A instruction selection
   - Register allocation (linear scan)
   - AAPCS calling convention implementation
   - Stack frame management

5. **usage_guide.md**: Usage guide chi tiết
   - Command-line options
   - Debug features
   - Examples và best practices

Xem [CrossCompiler/README.md](CrossCompiler/README.md) để biết thêm chi tiết về compiler.

