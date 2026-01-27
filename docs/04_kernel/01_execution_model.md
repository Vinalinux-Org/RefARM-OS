# Kernel Execution Model

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **mô hình thực thi (execution model)** của kernel trong dự án RefARM-OS trên nền AM335x.

Mục tiêu của execution model là:

- Xác định kernel chạy như thế nào sau khi nhận quyền điều khiển từ boot stage.
- Làm rõ các pha thực thi (execution phases) của kernel.
- Định nghĩa rõ những giả định kernel được phép và không được phép đưa ra.
- Tạo nền tảng tư duy cho các thiết kế tiếp theo (interrupt model, driver model).

Tài liệu này **không** mô tả chi tiết triển khai mã nguồn và **không** bao gồm các cơ chế nâng cao của hệ điều hành.

---

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi của execution model

Bắt đầu sau khi boot kết thúc, tức là từ thời điểm hàm `kernel_main()` bắt đầu thực thi.

### 2.2. Các giai đoạn trước đó

Các giai đoạn sau đã được định nghĩa trong các tài liệu boot và nằm **ngoài phạm vi** của tài liệu này:

- Boot ROM
- MLO / SPL
- U-Boot (vendor)
- Reset sequence (`_start` → `reset.S` → `entry.S`)

### 2.3. Độc lập với bootloader

Execution model này **không phụ thuộc** vào việc bootloader là vendor hay custom, miễn là **boot contract** được đảm bảo.

---

## 3. Giả định kiến trúc (Architectural Assumptions)

Kernel execution model trong dự án này dựa trên các giả định sau:

### 3.1. Single-core execution

- Kernel giả định môi trường thực thi **single-core**.
- Không có core khác hoạt động song song.
- Không có concurrency giữa nhiều CPU.

> **This kernel assumes a single-core execution environment. SMP is explicitly out of scope.**

Giả định này phản ánh:
- Thực tế phần cứng (Cortex-A8 là single-core).
- Đồng thời giữ execution model đơn giản và rõ ràng.

### 3.2. Privileged execution

- Kernel luôn chạy ở **privileged mode (SVC)**.
- Không có user mode.
- Không có chuyển đổi privilege level trong phạm vi hiện tại.

### 3.3. Flat execution environment

- **MMU đang tắt** (MMU OFF).
- Kernel chạy trực tiếp trên **physical address space**.
- Không có khái niệm virtual address trong execution model hiện tại.

---

## 4. Khái niệm execution flow trong kernel

Trong RefARM-OS, kernel **không có** thread, task hay process theo nghĩa hệ điều hành truyền thống.

Thay vào đó, kernel được mô tả thông qua các **pha thực thi (execution phases)**.

Các execution phase:
- Không đại diện cho context switch.
- Không song song.

---

## 5. Các pha thực thi của kernel

### 5.1. Early Kernel Phase

Đây là pha bắt đầu ngay khi `kernel_main()` được gọi.

**Đặc điểm:**
- Kernel vừa mới bước vào C code.
- Môi trường runtime ở mức tối thiểu.
- Interrupt vẫn đang bị mask.
- Chỉ cho phép các thao tác an toàn, deterministic.

**Mục tiêu của pha này:**
- Xác nhận kernel đã chạy đúng.
- Thiết lập trạng thái nội bộ tối thiểu của kernel.
- Chuẩn bị điều kiện tối thiểu để kernel có thể tiến hành initialization một cách an toàn.

### 5.2. Kernel Initialization Phase

Pha này chịu trách nhiệm:
- Thiết lập các cấu trúc nội bộ của kernel.
- Khởi tạo các subsystem ở mức kiến trúc (chưa phải triển khai đầy đủ).

**Ví dụ:**
- Thiết lập vector table và exception infrastructure (interrupt vẫn bị mask).
- Khởi tạo driver framework cơ bản (chỉ polling I/O, chưa có interrupt-driven I/O).
- Thiết lập trạng thái kernel sẵn sàng đi vào steady execution.

**Trong pha này:**
- Kernel **không** bật interrupt.
- Kernel **không** chạy song song bất kỳ luồng nào khác.

### 5.3. Steady Execution Phase

Đây là pha mà kernel được thiết kế để hoạt động lâu dài.

**Đặc điểm về mặt mô hình:**
- Kernel đã hoàn tất initialization.
- Kernel được thiết kế để có thể chạy với interrupt enabled.
- Kernel tiếp tục thực thi trong một vòng lặp hoặc trạng thái ổn định.

> **In the steady execution phase, the kernel is designed to eventually operate with interrupts enabled. In the current stage, interrupts remain disabled.**

Pha này là:
- Điểm neo cho các thiết kế tương lai (interrupt handling, timer, v.v.).
- **Không** đồng nghĩa với việc các cơ chế đó đã được triển khai ngay.

---

## 6. Vai trò của `kernel_main()`

`kernel_main()` là:
- **Entry point** của kernel C code.
- Điểm kết thúc của boot stage.
- Điểm bắt đầu của kernel execution model.

`kernel_main()` **không** đại diện cho vòng đời đầy đủ của kernel, mà chỉ là điểm khởi đầu của kernel execution model.

**Vai trò của `kernel_main()`:**
- Thực hiện initialization sớm của kernel.
- Thiết lập trạng thái kernel nội bộ.
- Chuyển kernel sang steady execution phase.

**`kernel_main()` không:**
- Đại diện cho thread hoặc task.
- Thực hiện context switch.
- Kết thúc vòng đời kernel.

---

## 7. Interrupt và Exception trong execution model

Trong execution model này:
- **Interrupt** được xem là **sự kiện bất đồng bộ** có thể ảnh hưởng tới kernel flow.
- **Exception** (sự kiện đồng bộ) có thể xảy ra trong bất kỳ execution phase nào.
- Execution model cho phép tư duy về interrupt trong steady phase.

Tuy nhiên, trong trạng thái hiện tại:
- Interrupt vẫn đang bị mask.
- Không có interrupt handler thực thi.

Chi tiết được định nghĩa trong:
- Interrupt handling: **`04_kernel/02_interrupt_model.md`**
- Exception handling: **`04_kernel/03_exception_model.md`**

---

## 8. Ranh giới trách nhiệm của kernel execution model

### Execution model chịu trách nhiệm:
- Mô tả kernel chạy như thế nào về mặt kiến trúc.
- Định nghĩa các execution phase.
- Làm rõ các giả định và giới hạn.

### Execution model không chịu trách nhiệm:
- Mô tả chi tiết exception entry (xem `04_kernel/03_exception_model.md`).
- Mô tả scheduler hoặc preemption.
- Mô tả quản lý bộ nhớ nâng cao.
- Mô tả user space.

---

## 9. Explicit Non-Goals

Execution model này **explicitly does NOT include**:

- User mode
- Process hoặc thread abstraction
- Scheduler hoặc preemption
- Virtual memory hoặc MMU usage
- Memory allocator
- SMP hoặc multi-core execution
- IPC hoặc system call

Mọi nội dung nằm ngoài danh sách trên đều **không được giả định** khi đọc tài liệu này.

---

## 10. Quan hệ với các tài liệu khác

### Execution model này là nền tảng cho:
- `04_kernel/02_interrupt_model.md`
- `04_kernel/03_exception_model.md`
- `04_kernel/05_driver_model.md` (future work)

### Execution model phụ thuộc vào:
- `02_boot/boot_contract.md`
- `02_boot/boot_flow.md`
- `02_boot/reset_sequence.md`

Mọi thay đổi làm ảnh hưởng đến **execution assumptions** phải được phản ánh đồng bộ trong các tài liệu liên quan.

---

## 11. Ghi chú thiết kế

- Execution model phải **đơn giản, rõ ràng và deterministic**.
- Không mở rộng execution model chỉ để "đón đầu" các tính năng chưa có.
- Mọi mở rộng phải xuất phát từ **nhu cầu kiến trúc rõ ràng**, không phải từ thói quen của hệ điều hành khác.