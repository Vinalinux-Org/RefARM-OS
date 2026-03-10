# PROJECT_SCOPE

RefARM Reference Platform

---

## 1. Tổng quan dự án

Dự án này nhằm xây dựng một Reference ARM Software Platform hoàn chỉnh chạy trực tiếp trên phần cứng thật, bao gồm:
- **Boot & Bring-up Layer**
- **Board Support Package (BSP)**
- **Minimal Reference Operating System**
- **Userspace** (User Applications + Runtime + Build Infrastructure)

Nền tảng mục tiêu:
- Board: BeagleBone Black
- SoC: Texas Instruments AM3358
- Kiến trúc CPU: ARMv7-A (Cortex-A8)

Board này được xem như một SoC mới hoàn toàn, không sử dụng Linux BSP hoặc SDK thương mại có sẵn.
Dự án không nhằm mục tiêu thương mại, không thay thế Linux hoặc RTOS hiện có.

Mục tiêu của dự án là tạo một reference platform phục vụ:
- Phân tích kiến trúc SoC.
- Hiểu rõ tương tác OS–Hardware.
- Làm nền tảng kỹ thuật cho phát triển chip ARM trong tương lai.
- **Cung cấp target hardware thực tế cho compiler tự phát triển ở Phase 2.**

### Lộ trình tổng thể

```text
Phase 1: RefARM Platform
   Boot + BSP + Minimal OS + Userspace
   ↓
Phase 2: Self-hosted Compiler
   Tự xây dựng compiler nhắm đến ARMv7-A
   Target = chính platform đã xây ở Phase 1
```

---

## 2. Mục tiêu kỹ thuật

Dự án phải đạt được các mục tiêu sau:
- Làm rõ toàn bộ chuỗi thực thi từ BootROM handoff đến kernel hoạt động.
- Xây dựng một OS reference tối giản nhưng đúng kiến trúc ARMv7-A.
- Xây dựng userspace tối giản: user application chạy độc lập ở User Mode, giao tiếp với kernel qua syscall.
- Cung cấp công cụ đo lường để validate hành vi phần cứng.
- Tài liệu hóa toàn bộ hệ thống đủ để tái triển khai độc lập.
- **Xây dựng nền tảng hiểu biết thực tế về ABI, calling convention, memory layout — làm cơ sở cho Phase 2.**

---

## 3. Kiến trúc hệ thống

Hệ thống được phân thành 4 tầng logic:
```text
Silicon / SoC
   ↓
Boot & Bring-up
   ↓
Reference OS Kernel
   ↓
User Application (via Userspace)
```

---

## 4. Phạm vi kỹ thuật

### 4.1. Boot & Bring-up Layer
**Mục tiêu:** kiểm soát CPU ngay sau reset vector.
**Bao gồm:**
- `start.S` (reset entry)
- Vector table setup
- CPU mode initialization
- Stack initialization cho các mode
- UART early console
- Timer initialization
- Interrupt controller configuration

**Tiêu chí hoàn thành:**
- Board boot thành công từ reset vector.
- In được log sớm qua UART.
- Có khả năng xử lý interrupt cơ bản.

### 4.2. Board Support Package (BSP)
BSP trong dự án này bao gồm:
- Định nghĩa register và memory map dựa trên TRM.
- Driver tối thiểu cho:
  - UART
  - Timer
  - Interrupt controller
- Linker script ánh xạ chính xác SRAM/DDR.
- Runtime startup (`crt0`).

*Lưu ý:* Không sử dụng high-level SDK từ Linux. Việc định nghĩa register phải dựa trực tiếp vào tài liệu kỹ thuật SoC (TRM).

### 4.3. Reference Operating System
Kernel dạng monolithic tối giản.

**4.3.1. Execution Model**
- Single-core.
- Kernel mode (privileged).
- User mode cơ bản.
- Phân tách kernel/user ở mức MMU mapping đơn giản.

**4.3.2. Thành phần bắt buộc**
- Exception handling (Undefined, Prefetch Abort, Data Abort, IRQ, SVC).
- MMU enable với static mapping.
- Kernel/User memory split cơ bản.
- PCB (Process Control Block).
- Scheduler (round-robin).
- Context switch (save/restore register).
- System call interface thông qua SVC.
- Tập syscall tối thiểu: `write`, `exit`, `yield`.

**4.3.3. Thành phần minh họa kiến trúc**
- Shell dòng lệnh tối giản qua UART.
- **ramfs tối giản có VFS layer đơn giản:**
  - `ramfs_mount("/")` — mount point tại boot.
  - Files nhúng sẵn vào kernel image lúc build (qua `payload.S`).
  - VFS interface: `vfs_read()`, `vfs_ls()`.
  - Shell commands: `ls` (liệt kê files), `cat <file>` (đọc nội dung file).
- Không hướng tới POSIX compatibility.

### 4.4. Userspace
**Mục tiêu:** Xây dựng user application chạy độc lập ở User Mode trên RefARM OS — tách biệt hoàn toàn khỏi kernel image. Đây là "userland" tối giản của platform, tương đương với userspace trong Linux.

**4.4.1. Build Toolchain**
- Cross-compiler cho ARMv7-A dựa trên GCC / binutils.
- Dùng trực tiếp toolchain có sẵn trên máy host (không cần đóng gói riêng).
- *Không phát triển compiler mới ở Phase 1. Compiler tự phát triển thuộc Phase 2.*

**4.4.2. User Runtime**
- `crt0.S` — startup code, setup stack và zero BSS trước khi gọi `main()`.
- Linker script (`app.ld`) — map binary vào User Space (`0x40000000`).
- Syscall wrapper (`syscalls.h`, `user_syscall.h`) — giao tiếp với kernel qua `SVC #0`.

**4.4.3. Build Infrastructure**
- Makefile chuẩn cho user application.
- Script flash SD card.

**4.4.4. User Applications**
- `shell` — chạy ở User Mode (`0x40000000`), giao tiếp qua UART, gọi syscall.

**Tiêu chí:** Shell compile độc lập, load vào `0x40000000`, chạy ở User Mode, giao tiếp với kernel qua syscall.



## 5. Ngoài phạm vi (Phase 1)

Dự án Phase 1 không bao gồm:
- POSIX đầy đủ.
- Networking stack.
- Full filesystem.
- SMP / multi-core.
- Power management nâng cao.
- Security framework phức tạp.
- **Phát triển compiler mới** *(chuyển sang Phase 2 — sau khi hoàn thành Phase 1).*
- Thiết kế ISA mới.

---

## 6. Nguyên tắc thiết kế

- Mỗi module phải có mục đích rõ ràng.
- Không thêm abstraction không cần thiết.
- Ưu tiên tính minh bạch và khả năng trace.
- Code phải phản ánh kiến trúc.
- Mọi subsystem phải có tài liệu kèm theo.

---

## 7. Vai trò của tài liệu

Tài liệu là thành phần bắt buộc của sản phẩm. Mỗi subsystem phải có:
- Kiến trúc tổng quan.
- Flow xử lý.
- Diagram minh họa.
- Memory layout.
- Giải thích quyết định thiết kế.

Một kỹ sư khác phải có khả năng tái triển khai hệ thống dựa trên tài liệu và source code.

---

## 8. Tiêu chí hoàn thành (Phase 1)

Dự án Phase 1 được coi là hoàn thành khi:
- Boot thành công trên board thật.
- Kernel xử lý interrupt ổn định.
- `idle` task chạy ở Kernel Mode.
- `shell` task chạy độc lập ở User Mode (`0x40000000`), preemptive với `idle`.
- MMU hoạt động theo mapping thiết kế.
- Shell tương tác mượt mà qua UART, gọi đúng Syscall ABI.
- ramfs hoạt động: `ls` và `cat` chạy được trong shell.
- Có tài liệu kỹ thuật đầy đủ.

---

## 9. Phase 2 – Self-hosted Compiler

> **Trạng thái:** Planned — bắt đầu sau khi Phase 1 hoàn thành.

### 9.1. Mục tiêu

Tự phát triển một compiler hoàn chỉnh nhắm đến kiến trúc ARMv7-A, sử dụng chính platform RefARM (Phase 1) làm target hardware để chạy và validate output.

Mục tiêu cốt lõi:
- Hiểu sâu toàn bộ pipeline compiler: từ source text đến binary chạy được trên hardware thật.
- Không phụ thuộc LLVM hoặc GCC backend (tự xây dựng toàn bộ).
- Compiler output phải chạy được trực tiếp trên BeagleBone Black qua RefARM OS.
- Tài liệu hóa đầy đủ từng giai đoạn compile.

### 9.2. Kiến trúc compiler

```text
Source Code
   ↓
[Lexer] — tokenization
   ↓
[Parser] — syntax analysis → AST
   ↓
[Semantic Analyzer] — type checking, symbol resolution
   ↓
[IR Generator] — intermediate representation
   ↓
[Optimizer] — optional, basic passes
   ↓
[Code Generator] — emit ARMv7-A assembly
   ↓
[Assembler / Linker] — produce ELF binary
   ↓
Binary chạy trên RefARM Platform
```

### 9.3. Quyết định kỹ thuật

| Hạng mục | Quyết định | Lý do |
|---|---|---|
| **Ngôn ngữ implement compiler** | **C** | Dùng xuyên suốt Phase 1. Tài liệu tham khảo phong phú nhất. Gần với tư duy assembly khi debug. Con đường tự nhiên đến self-hosting. |
| **Ngôn ngữ nguồn (source language)** | **Subset of C** | Đủ để học toàn bộ pipeline. Có thể so sánh output với GCC. Không mất thời gian thiết kế ngôn ngữ mới. |
| **Backend** | **Tự viết toàn bộ (không LLVM)** | Mục tiêu là hiểu sâu — LLVM sẽ che khuất phần codegen quan trọng nhất. |
| **IR format** | **3-address code** | Đơn giản hơn SSA, dễ implement cho người mới, đủ để học codegen thực sự. |
| **Output format** | **ELF** | Tương thích RefARM loader và linker script từ Phase 1. |

**Subset C bao gồm tối thiểu:**
- Kiểu dữ liệu: `int`, `char`, `pointer`
- Cấu trúc điều khiển: `if/else`, `while`, `for`, `return`
- Hàm: định nghĩa, gọi hàm, đệ quy
- Mảng và con trỏ cơ bản
- Syscall thông qua inline hoặc thư viện tối giản

**Không bao gồm (ngoài phạm vi Subset C):**
- `struct`, `union`, `enum`
- `float`, `double`
- Preprocessor (`#include`, `#define`)
- Standard library đầy đủ

*Các quyết định trên được ghi lại chính thức. Nếu thay đổi trong quá trình thực hiện, cần cập nhật tài liệu ADR riêng.*

### 9.4. Thành phần bắt buộc

**9.4.1. Lexer**
- Tokenization từ source text.
- Xử lý keywords, identifiers, literals, operators.
- Error reporting với line/column.

**9.4.2. Parser**
- Recursive descent hoặc Pratt parser.
- Sinh Abstract Syntax Tree (AST).
- Error recovery cơ bản.

**9.4.3. Semantic Analyzer**
- Symbol table và scoping.
- Type checking.
- Resolve references.

**9.4.4. IR (Intermediate Representation)**
- Dạng trung gian độc lập với hardware.
- Đủ để thực hiện basic optimization.
- Có thể dump để debug.

**9.4.5. Code Generator — ARMv7-A Backend**
- Emit ARMv7-A assembly (Thumb-2 optional).
- Register allocation (naive hoặc linear scan).
- AAPCS calling convention — dựa trực tiếp trên kinh nghiệm Phase 1.
- Stack frame management.
- Syscall emit tương thích RefARM syscall ABI.

**9.4.6. Assembler & Linker**
- Tự viết assembler tối giản hoặc tích hợp với GNU as.
- Linker tạo ELF binary tương thích RefARM memory map.
- Linker script tái sử dụng từ Phase 1 SDK.

### 9.5. Mối liên hệ với Phase 1

Phase 1 cung cấp cho Phase 2:

| Phase 1 output | Phase 2 sử dụng như |
|---|---|
| ARMv7-A register knowledge | Codegen target |
| AAPCS calling convention (thực hành) | ABI cho compiler output |
| Syscall interface (`write`, `exit`, `yield`) | Runtime syscall emit |
| Linker script & memory map | Linker configuration |
| RefARM OS + Board | Execution environment để test compiler output |
| Measurement framework | Validate compiler output performance |

### 9.6. Example Programs (Phase 2)

- Hello world được compile bởi compiler tự viết, chạy trên RefARM.
- Chương trình đệ quy (kiểm tra stack frame và calling convention).
- Chương trình có syscall (kiểm tra ABI tương thích).
- Self-hosting test (compiler tự compile một phần của chính nó — mục tiêu dài hạn, optional).

### 9.7. Validation

- Output binary chạy đúng trên BeagleBone Black qua RefARM OS.
- So sánh output assembly với GCC -O0 để kiểm tra tính đúng đắn.
- Đo performance overhead so với GCC output.
- Tất cả kết quả ghi lại trong tài liệu kỹ thuật.

### 9.8. Tiêu chí hoàn thành (Phase 2)

- Compiler build được chương trình Subset C cơ bản (int, pointer, if/while, function call).
- Output binary chạy đúng trên RefARM Platform (BeagleBone Black).
- Syscall tương thích với RefARM OS.
- Có tài liệu kỹ thuật đầy đủ cho từng component compiler.
- Có kết quả so sánh với GCC output.

---

**KẾT LUẬN**

Dự án này là một:
> **ARM Reference Software Platform (Phase 1)**
> Bao gồm Boot + BSP + Minimal OS + Userspace.

Tiếp theo là:
> **Self-hosted Compiler (Phase 2)**
> Tự xây compiler nhắm ARMv7-A, chạy output trên chính platform Phase 1.

Không phải research compiler tách rời. Không phải commercial OS. Không phải Linux replacement.
Platform Phase 1 là nền tảng — compiler Phase 2 là đỉnh của lộ trình.