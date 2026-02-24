# PROJECT_SCOPE

RefARM Reference Platform
Version 1.0

## 1. Tổng quan dự án

Dự án này nhằm xây dựng một Reference ARM Software Platform hoàn chỉnh chạy trực tiếp trên phần cứng thật, bao gồm:
- **Boot & Bring-up Layer**
- **Board Support Package (BSP)**
- **Minimal Reference Operating System**
- **Development SDK** (Toolchain + Runtime Template + Build Infrastructure)
- **Validation & Measurement Framework**

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

## 2. Mục tiêu kỹ thuật

Dự án phải đạt được các mục tiêu sau:
- Làm rõ toàn bộ chuỗi thực thi từ BootROM handoff đến kernel hoạt động.
- Xây dựng một OS reference tối giản nhưng đúng kiến trúc ARMv7-A.
- Cung cấp SDK cho phép kỹ sư khác phát triển và build ứng dụng cho nền tảng.
- Cung cấp công cụ đo lường để validate hành vi phần cứng.
- Tài liệu hóa toàn bộ hệ thống đủ để tái triển khai độc lập.

## 3. Kiến trúc hệ thống

Hệ thống được phân thành 4 tầng logic:
```text
Silicon / SoC
   ↓
Boot & Bring-up
   ↓
Reference OS Kernel
   ↓
User Application (via SDK)
```

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
- File abstraction mức tối thiểu (in-memory hoặc block đơn giản).
- Không hướng tới POSIX compatibility.

### 4.4. Development SDK
**Mục tiêu:** Cho phép kỹ sư phát triển và build chương trình cho nền tảng.

**4.4.1. Toolchain**
- Cross-compiler cho ARMv7-A.
- Dựa trên GCC / binutils / newlib.
- Được build hoặc đóng gói riêng cho dự án. Có sysroot riêng.
- *Không phát triển compiler mới.*

**4.4.2. Runtime Template**
- `crt0`/startup template cho user application.
- Linker script mẫu.
- Syscall header.
- Register header tự định nghĩa.

**4.4.3. Build & Flash Infrastructure**
- Makefile/CMake chuẩn.
- Script build image.
- Script format và copy SD card.
- Script flash tiện dụng.

**4.4.4. Example Programs**
- Hello world.
- Multitask demo.
- Syscall demo.

**Yêu cầu SDK:** Một kỹ sư khác có thể: `Clone → build → flash → run`.

### 4.5. Validation & Measurement
**Mục tiêu:** Dùng OS làm công cụ kiểm chứng SoC.
**Bao gồm:**
- Interrupt latency measurement.
- Context switch timing.
- Syscall overhead measurement.
- Timer accuracy verification.
- Basic MMU behavior test.

*Kết quả phải được ghi lại trong tài liệu kỹ thuật.*

## 5. Ngoài phạm vi bao gồm

Dự án không bao gồm:
- POSIX đầy đủ.
- Networking stack.
- Full filesystem.
- SMP / multi-core.
- Power management nâng cao.
- Security framework phức tạp.
- Phát triển compiler mới.
- Thiết kế ISA mới.

## 6. Nguyên tắc thiết kế

- Mỗi module phải có mục đích rõ ràng.
- Không thêm abstraction không cần thiết.
- Ưu tiên tính minh bạch và khả năng trace.
- Code phải phản ánh kiến trúc.
- Mọi subsystem phải có tài liệu kèm theo.

## 7. Vai trò của tài liệu

Tài liệu là thành phần bắt buộc của sản phẩm. Mỗi subsystem phải có:
- Kiến trúc tổng quan.
- Flow xử lý.
- Diagram minh họa.
- Memory layout.
- Giải thích quyết định thiết kế.

Một kỹ sư khác phải có khả năng tái triển khai hệ thống dựa trên tài liệu và source code.

## 8. Tiêu chí hoàn thành

Dự án được coi là hoàn thành khi:
- Boot thành công trên board thật.
- Kernel xử lý interrupt ổn định.
- Có ít nhất hai user task chạy preemptive.
- MMU hoạt động theo mapping thiết kế.
- SDK build được user application.
- Có tài liệu kỹ thuật đầy đủ.
- Có kết quả measurement xác thực.

---
**KẾT LUẬN**

Dự án này là một:
> **ARM Reference Software Platform**
> Bao gồm Boot + BSP + Minimal OS + SDK + Validation.

Không phải research compiler. Không phải commercial OS. Không phải Linux replacement.
