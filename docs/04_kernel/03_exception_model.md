# Kernel Exception Model

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **mô hình exception (exception model)** của kernel trong dự án RefARM-OS trên nền AM335x.

Mục tiêu của exception model là:

- Xác định vai trò của exception trong kiến trúc kernel.
- Định nghĩa cách kernel tư duy về exception và phân biệt với interrupt.
- Làm rõ ranh giới trách nhiệm giữa CPU, vector table, exception entry và kernel handler.
- Chuẩn bị nền tảng tư duy cho việc triển khai exception handling.

Tài liệu này **không** mô tả chi tiết mã nguồn xử lý exception và **không** triển khai đầy đủ exception handler trong giai đoạn hiện tại.

---

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi của exception model

Exception model áp dụng cho kernel sau khi boot kết thúc, tức là từ thời điểm `kernel_main()` bắt đầu thực thi.

### 2.2. Các chi tiết sau nằm ngoài phạm vi của tài liệu này

- Vector table assembly implementation chi tiết
- Exception entry/exit assembly code
- Stack frame layout cụ thể
- Exception recovery mechanism
- Nested exception handling

### 2.3. Exception model này độc lập với

- Bootloader cụ thể
- Cách vector table được load vào memory
- Interrupt controller (INTC) configuration

---

## 3. Định nghĩa exception trong RefARM-OS

### 3.1. Exception là gì?

Trong phạm vi RefARM-OS:

**Exception là:**
- Sự kiện làm gián đoạn normal execution flow của CPU.
- Được xử lý bởi CPU hardware thông qua vector table.
- Có thể là synchronous (do instruction gây ra) hoặc asynchronous (do hardware signal).

**Exception không:**
- Là cơ chế scheduling.
- Là cơ chế IPC.
- Là software-only construct.

### 3.2. Exception vs Interrupt

**Phân biệt quan trọng:**

| Aspect | Exception | Interrupt |
|--------|-----------|-----------|
| Source | Instruction execution | Hardware peripheral |
| Timing | Synchronous | Asynchronous |
| Predictable | Yes (for most) | No |
| Examples | Undefined instr, Data abort | UART IRQ, Timer IRQ |

**Trong RefARM-OS:**
- IRQ và FIQ là **exception types** theo ARM terminology.
- Nhưng về mặt kernel model, chúng được xử lý theo **interrupt model**.
- Tài liệu này tập trung vào synchronous exceptions (Undefined, SVC, Abort).

---

## 4. Architectural Assumptions

Exception model trong dự án này dựa trên các giả định sau:

### 4.1. Single-core exception handling

- Exception chỉ xảy ra trên **single core**.
- Không có cross-core exception routing.
- Exception handler thực thi tuần tự, không song song.

### 4.2. Privileged-only exception context

- Mọi exception đều được xử lý trong **privileged mode**.
- Kernel luôn chạy ở SVC mode, không có user mode.
- Không có privilege escalation cần xử lý.

### 4.3. No nested exceptions (current phase)

Trong giai đoạn hiện tại:
- Exception **không** được phép lồng nhau.
- Khi đang xử lý exception, các exception khác sẽ bị mask hoặc dẫn đến undefined behavior.

> **Nested exception handling is explicitly out of scope for the current phase.**

### 4.4. Vector table placement

**AM335x vector table strategy:**
- Reset vector: `0x4030CE00` (Boot ROM, out of kernel scope)
- Kernel vector table: Programmable via **VBAR (Vector Base Address Register)**

**Trong RefARM-OS:**
- Vector table được đặt trong kernel image (ví dụ: `0x80000000`)
- VBAR được cấu hình trong boot stage (`entry.S`)
- Kernel không sử dụng fixed low vectors (`0x00000000`)

**Lý do:**
- AM335x reset behavior: Boot ROM owns address `0x00000000`
- Kernel cần flexible vector table placement
- VBAR cho phép relocatable kernel image

---

## 5. ARM Exception Types

ARM architecture định nghĩa 8 exception types. Mỗi exception có **offset cố định** từ Vector Base Address Register (VBAR).

### 5.1. Reset Exception

**Mode:** Supervisor (SVC)  
**Offset:** +0x00 (từ VBAR)  
**Entry:** `VBAR + 0x00`

**Đặc điểm:**
- Xảy ra khi CPU reset hoặc power-on.
- Là điểm bắt đầu của boot sequence.

**Trên AM335x:**
- Reset vector thực tế: `0x4030CE00` (Boot ROM)
- Kernel **không bao giờ** thấy reset exception tại offset 0x00

**Trong RefARM-OS:**
- Reset exception được xử lý bởi **boot stage** (`reset.S`).
- **Nằm ngoài phạm vi** của kernel exception model.

### 5.2. Undefined Instruction Exception

**Mode:** Undefined (UND)  
**Offset:** +0x04  
**Entry:** `VBAR + 0x04`

**Xảy ra khi:**
- CPU gặp instruction không hợp lệ hoặc không được hỗ trợ.
- Cố gắng thực thi coprocessor instruction không có.

**Trong RefARM-OS:**
- Được xem là **fatal error** trong giai đoạn hiện tại.
- Handler sẽ halt kernel với diagnostic message.

### 5.3. Software Interrupt (SVC) Exception

**Mode:** Supervisor (SVC)  
**Offset:** +0x08  
**Entry:** `VBAR + 0x08`

**Xảy ra khi:**
- Thực thi instruction `SVC` (Software interrupt, cũ gọi là `SWI`).

**Trong RefARM-OS:**
- **Không sử dụng** trong giai đoạn hiện tại (no user mode, no syscall).
- Được dự trữ cho tương lai nếu có system call interface.

### 5.4. Prefetch Abort Exception

**Mode:** Abort (ABT)  
**Offset:** +0x0C  
**Entry:** `VBAR + 0x0C`

**Xảy ra khi:**
- Instruction fetch từ địa chỉ không hợp lệ.
- Memory access violation khi fetch instruction.
- MMU permission fault (ví dụ: nhảy vào vùng Execute Never).

**Trên Cortex-A8 (AM335x):**
- Prefetch Abort có thể do alignment fault.
- Prefetch Abort có thể do MMU translation fault.
- Prefetch Abort có thể do permission fault (NX bit violation).

**Trong RefARM-OS:**
- Được xem là **fatal error**.
- Handler sẽ halt kernel với diagnostic message.

### 5.5. Data Abort Exception

**Mode:** Abort (ABT)  
**Offset:** +0x10  
**Entry:** `VBAR + 0x10`

**Xảy ra khi:**
- Data access từ địa chỉ không hợp lệ.
- Memory access violation khi load/store.
- MMU permission fault hoặc translation fault.

**Trong RefARM-OS:**
- Được xem là **fatal error**.
- Handler sẽ halt kernel với diagnostic message.

### 5.6. Reserved Exception

**Offset:** +0x14  
**Entry:** `VBAR + 0x14`

**Không được sử dụng** trong ARM architecture.

### 5.7. IRQ Exception

**Mode:** IRQ  
**Offset:** +0x18  
**Entry:** `VBAR + 0x18`

**Xảy ra khi:**
- Hardware interrupt xảy ra và IRQ được enabled.

**Trong RefARM-OS:**
- Được xử lý theo **interrupt model** (`02_interrupt_model.md`).
- Trong giai đoạn hiện tại: IRQ bị mask.

### 5.8. FIQ Exception

**Mode:** FIQ  
**Offset:** +0x1C  
**Entry:** `VBAR + 0x1C`

**Xảy ra khi:**
- Fast interrupt xảy ra và FIQ được enabled.

**Trong RefARM-OS:**
- **Không sử dụng** trong giai đoạn hiện tại.
- FIQ luôn bị mask.

---

## 6. Exception Layering Model

Exception trong kernel được tổ chức theo các lớp logic rõ ràng:

```
[ Exception Event ]
        ↓
[ ARM CPU Hardware ]
        ↓
[ Vector Table ]
        ↓
[ Exception Entry (Assembly) ]
        ↓
[ Kernel Exception Handler (C) ]
```

Mỗi lớp có **trách nhiệm rõ ràng** và **không chồng chéo**.

### 6.1. Exception Event Layer

**Bao gồm:**
- Instruction execution triggering exception.
- Hardware signal (for IRQ/FIQ).

**Kernel không kiểm soát:**
- Khi nào exception xảy ra (trừ SVC instruction).

### 6.2. ARM CPU Hardware Layer

**Trách nhiệm của CPU:**
- Nhận diện exception type.
- Switch CPU mode (SVC, UND, ABT, IRQ, FIQ).
- Save return address vào `LR_<mode>`.
- Save CPSR vào `SPSR_<mode>`.
- Disable IRQ/FIQ (depending on exception type).
- Jump đến vector table entry.

**Kernel không can thiệp** vào layer này - đây là hardware behavior.

### 6.3. Vector Table Layer

**Trách nhiệm:**
- Routing: chuyển điều khiển từ fixed vector address đến exception entry code.
- Là **contract** giữa CPU và kernel.

**Vector table:**
- **Không** xử lý logic.
- **Không** save context.
- Chỉ là **jump table**.

Chi tiết trong: `04_vector_table_contract.md`.

### 6.4. Exception Entry Layer

**Trách nhiệm:**
- Save CPU context (registers, CPSR).
- Setup environment cho C handler.
- Call kernel exception handler.
- Restore CPU context.
- Exception return.

**Exception entry:**
- Viết bằng **assembly**.
- **Không** chứa logic nghiệp vụ.
- Là bridge giữa CPU hardware và C code.

### 6.5. Kernel Exception Handler Layer

**Trách nhiệm:**
- Xử lý exception theo logic kernel.
- Print diagnostic information.
- Decide recovery hoặc halt.

**Kernel handler:**
- Viết bằng **C**.
- **Không** biết chi tiết CPU mode switching.
- Nhận context đã được chuẩn hóa.

---

## 7. Exception Context

Khi exception xảy ra, CPU và kernel bước vào **exception context**.

### 7.1. CPU State Changes

Khi exception occurs:
1. CPU switches to exception mode (UND/SVC/ABT/IRQ/FIQ).
2. `PC` của instruction gây exception được save vào `LR_<mode>`.
3. `CPSR` được save vào `SPSR_<mode>`.
4. CPU jumps đến vector table entry.

### 7.2. Stack Requirements

Mỗi exception mode có **dedicated stack**:
- **UND_stack**: for Undefined Instruction handler
- **ABT_stack**: for Data/Prefetch Abort handler
- **IRQ_stack**: for IRQ handler
- **FIQ_stack**: for FIQ handler (not used)

**SVC mode:** sử dụng kernel main stack (đã setup trong boot).

### 7.3. Register Preservation

Exception entry code **must preserve**:
- All general-purpose registers (r0-r12)
- User stack pointer (r13_usr)
- User link register (r14_usr)
- SPSR (contains CPSR before exception)

---

## 8. Exception Handling Rules

Để giữ kernel đơn giản và an toàn, exception model đặt ra các quy tắc sau:

### 8.1. Exception handler phải deterministic

- Exception handler **không được** phụ thuộc vào state không xác định.
- Exception handler **không được** gây ra exception khác (trừ debug exceptions).

### 8.2. Exception handler không giả định high-level runtime

Trong exception context:
- **Không** cấp phát bộ nhớ động.
- **Không** gọi blocking operations.
- **Không** assume interrupts enabled.

### 8.3. Fatal exceptions halt the kernel

Trong giai đoạn hiện tại:
- Undefined Instruction → **halt with diagnostics**
- Prefetch Abort → **halt with diagnostics**
- Data Abort → **halt with diagnostics**

**Không có exception recovery mechanism.**

### 8.4. Exception priority

Nếu multiple exceptions xảy ra đồng thời, ARM hardware có priority order:
1. Reset (highest)
2. Data Abort
3. FIQ
4. IRQ
5. Prefetch Abort
6. Undefined / SVC (lowest)

**Kernel không can thiệp** vào priority này.

---

## 9. Quan hệ với execution model và interrupt model

### 9.1. Với execution model

- Exception chỉ có ý nghĩa trong context của **kernel execution**.
- Exception có thể xảy ra trong bất kỳ execution phase nào.
- Exception handler **không** thay đổi execution phase.

### 9.2. Với interrupt model

- IRQ/FIQ là exception types theo ARM terminology.
- Nhưng được xử lý theo **interrupt model**, không phải exception model.
- Exception model tập trung vào **synchronous exceptions**.

---

## 10. Exception State Machine

```
[Normal Execution]
        ↓ (exception occurs)
[Exception Entry]
        ↓ (context saved)
[Exception Handler]
        ↓ (handler completes)
[Exception Exit]
        ↓ (context restored)
[Normal Execution]  (or HALT for fatal exceptions)
```

**Quan trọng:**
- Exception **không** là một execution phase riêng biệt.
- Exception là **interruption** của normal execution.

---

## 11. Explicit Non-Goals

Exception model này **explicitly does NOT include**:

- Nested exception handling
- Exception recovery mechanism
- User-mode exception handling
- Exception-based scheduling
- Software exception emulation
- Coprocessor exception handling
- Debug exception (breakpoint, watchpoint)

Mọi nội dung nằm ngoài danh sách trên đều **không được giả định** khi đọc tài liệu này.

---

## 12. Quan hệ với các tài liệu khác

### Exception model này:
- Dựa trên `04_kernel/01_execution_model.md`
- Song song với `04_kernel/02_interrupt_model.md`
- Là nền tảng cho `04_kernel/04_vector_table_contract.md`

### Exception model phụ thuộc vào:
- `02_boot/reset_sequence.md` (reset exception)
- `02_boot/boot_contract.md` (CPU state assumptions)

### Exception model không thay thế:
- Boot-time exception handling
- Architecture-specific exception entry code documentation

Mọi thay đổi làm ảnh hưởng đến **exception assumptions** phải được phản ánh đồng bộ trong các tài liệu liên quan.

---

## 13. Ghi chú thiết kế

- Exception model phải **đơn giản, rõ ràng và deterministic**.
- Không mở rộng exception model để "đón đầu" các tính năng chưa có.
- Mọi mở rộng phải xuất phát từ **nhu cầu kiến trúc rõ ràng**.
- Exception handling **không được** trở thành cơ chế "workaround" cho kernel design issues.

---

## 14. Current Implementation Status

**Giai đoạn hiện tại (Pre-kernel architecture phase):**
- DEFINED: Exception model được định nghĩa rõ ràng
- TODO: Vector table chưa được implement
- TODO: Exception entry stubs chưa được implement
- TODO: Exception handlers chưa được implement

**Next steps:**
- Implement vector table theo `04_vector_table_contract.md`
- Implement exception entry stubs (assembly)
- Implement basic exception handlers (C)
- Test exception handling với deliberate invalid operations