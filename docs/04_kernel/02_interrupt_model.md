# Kernel Interrupt Model

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **mô hình interrupt (interrupt model)** của kernel trong dự án RefARM-OS.

Mục tiêu của interrupt model là:

- Xác định vai trò của interrupt trong kiến trúc kernel.
- Định nghĩa cách kernel tư duy và tổ chức interrupt, không phụ thuộc vào triển khai.
- Làm rõ ranh giới trách nhiệm giữa phần kiến trúc, kernel core và driver.
- Chuẩn bị nền tảng tư duy cho việc enable interrupt trong giai đoạn sau.

Tài liệu này **không** mô tả chi tiết mã nguồn xử lý interrupt và **không** bật interrupt trong trạng thái hiện tại.

---

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi của interrupt model

Interrupt model áp dụng cho kernel sau khi boot kết thúc, tức là từ thời điểm `kernel_main()` bắt đầu thực thi.

### 2.2. Các chi tiết sau nằm ngoài phạm vi của tài liệu này

- Vector table implementation chi tiết (xem `04_kernel/04_vector_table_contract.md`)
- Exception entry assembly (xem `04_kernel/03_exception_model.md`)
- IRQ/FIQ enable/disable trong code
- Scheduler hoặc preemption
- Nested interrupt

### 2.3. Interrupt model này độc lập với

- Bootloader cụ thể
- Cách interrupt controller (INTC) được lập trình chi tiết

---

## 3. Định nghĩa interrupt trong RefARM-OS

Trong phạm vi RefARM-OS:

**Interrupt là:**
- Sự kiện bất đồng bộ do phần cứng tạo ra.
- Có thể làm gián đoạn luồng thực thi hiện tại của kernel.
- Được điều phối bởi kernel, sau đó chuyển đến driver ISR.
- Driver không trực tiếp đăng ký interrupt với hardware.

**Interrupt không:**
- Là cơ chế scheduling
- Là cơ chế IPC
- Là công cụ quản lý concurrency nâng cao

---

## 4. Assumptions của interrupt model

Interrupt model trong dự án này dựa trên các giả định sau:

### 4.1. Single-core environment

- Kernel chạy trong môi trường **single-core**.
- Tại mọi thời điểm chỉ có **một execution flow được thực thi trên CPU**, ngoại trừ việc execution flow có thể bị gián đoạn tạm thời bởi interrupt.

**Không có:**
- Parallel interrupt handling
- Cross-core interrupt

### 4.2. Privileged-only interrupt handling

- Mọi interrupt đều được xử lý trong **privileged mode**.
- Không có user mode interrupt handling.
- Không có chuyển privilege level khi xử lý interrupt.

### 4.3. Interrupt bị mask trong giai đoạn hiện tại

Trong trạng thái hiện tại của dự án:
- IRQ và FIQ **chưa được enable**.
- Interrupt model được xây dựng cho tương lai, không ép triển khai ngay.

> **Interrupts are part of the architectural model, not necessarily part of the current runtime state.**

---

## 5. Mô hình phân tầng interrupt (Interrupt Layering)

Interrupt trong kernel được tổ chức theo các lớp logic rõ ràng:

```
[ Hardware Interrupt Source ]
              ↓
[ CPU Exception Entry ]
              ↓
[ Architecture-specific IRQ Layer ]
              ↓
[ Kernel Interrupt Core ]
              ↓
[ Driver-level ISR ]
```

Mỗi lớp có **trách nhiệm rõ ràng** và **không chồng chéo**.

---

## 6. Trách nhiệm của từng lớp

### 6.1. Hardware Interrupt Source

**Bao gồm:**
- Peripheral phát sinh interrupt (UART, timer, GPIO, …).
- Interrupt controller của SoC.

**Kernel không giả định:**
- Thứ tự ưu tiên cụ thể
- Cách mapping IRQ number chi tiết

Mọi chính sách ưu tiên interrupt đều được xem là **policy**, không thuộc execution model hoặc interrupt model.

### 6.2. CPU Exception Entry

**Đây là lớp:**
- Nhận quyền điều khiển ngay khi interrupt xảy ra.
- Chạy trong ngữ cảnh exception của CPU (IRQ/FIQ là exception types theo ARM).
- Xử lý hardware exception mechanism để chuyển vào kernel interrupt handling.

**Lưu ý:** IRQ và FIQ là exception types theo ARM architecture, nhưng được xử lý theo interrupt model trong kernel. Chi tiết về exception vs interrupt được làm rõ trong `04_kernel/03_exception_model.md`.

**Trách nhiệm:**
- Chuyển điều khiển vào kernel theo cách deterministic.

**Lớp này:**
- **Không** xử lý logic kernel.
- **Không** gọi driver.

### 6.3. Architecture-specific IRQ Layer

**Lớp này chịu trách nhiệm:**
- Chuẩn hóa exception entry thành sự kiện "IRQ" ở mức kernel.
- Ẩn chi tiết kiến trúc CPU (ARM) khỏi kernel core.

**Đặc điểm:**
- Phụ thuộc kiến trúc (ARM).
- Không chứa logic nghiệp vụ của kernel.

### 6.4. Kernel Interrupt Core

**Đây là trung tâm điều phối interrupt của kernel.**

**Trách nhiệm:**
- Nhận interrupt đã được chuẩn hóa.
- Xác định interrupt thuộc loại nào.
- Gọi handler tương ứng đã được đăng ký.

**Kernel interrupt core:**
- **Không** biết chi tiết hardware.
- **Không** trực tiếp thao tác peripheral.

### 6.5. Driver-level ISR

**ISR ở mức driver:**
- Xử lý sự kiện liên quan trực tiếp đến thiết bị.
- Thực hiện công việc tối thiểu cần thiết.

**Driver ISR:**
- **Không** điều phối interrupt toàn hệ thống.
- **Không** quyết định policy kernel.

---

## 7. Interrupt Context

Khi interrupt xảy ra, kernel bước vào **interrupt context**.

**Interrupt context có các đặc điểm:**
- Khác với execution flow thông thường của kernel.
- Có thể làm gián đoạn kernel code đang chạy.
- Phải được xử lý **nhanh** và **deterministic**.

**Interrupt context không phải:**
- Một thread
- Một task
- Một execution phase độc lập

---

## 8. Quy tắc xử lý interrupt (Interrupt Rules)

Để giữ kernel đơn giản và an toàn, interrupt model đặt ra các quy tắc sau:

### 8.1. ISR phải ngắn gọn

- ISR **không được** chứa logic phức tạp.
- ISR **không được** thực hiện xử lý kéo dài.

### 8.2. ISR không được giả định môi trường runtime cao cấp

Trong interrupt context:
- **Không** cấp phát bộ nhớ động.
- **Không** gọi scheduler.
- **Không** thực hiện blocking operation.

### 8.3. Không có nested interrupt (giai đoạn này)

- Kernel **không** giả định interrupt lồng nhau.
- Việc hỗ trợ nested interrupt (nếu có) sẽ được xem xét sau.

---

## 9. Quan hệ với execution model

Interrupt model phụ thuộc trực tiếp vào execution model:

- Interrupt handling chỉ được thiết kế cho **steady execution phase**.
- Trong early kernel và initialization phase:
  - Interrupt được mask và không được xử lý.
  - Interrupt hardware vẫn có thể phát tín hiệu, nhưng CPU không phản hồi do IRQ bit bị set.

Interrupt model **không** thay đổi execution model, mà chỉ định nghĩa cách execution model phản ứng trước các sự kiện bất đồng bộ.

---

## 10. Explicit Non-Goals

Interrupt model này **explicitly does NOT include**:

- Nested interrupt
- Preemption
- Interrupt-driven scheduling
- Softirq / tasklet
- Deferred work mechanism
- User-level interrupt handling
- SMP interrupt routing

Mọi nội dung ngoài danh sách trên **không được giả định**.

---

## 11. Quan hệ với các tài liệu khác

### Interrupt model này:
- Dựa trên `04_kernel/01_execution_model.md`
- Là nền tảng cho `04_kernel/05_driver_model.md` (future work)

### Interrupt model không thay thế:
- Boot interrupt masking trong `02_boot/reset_sequence.md`
- Vector table contract trong `04_kernel/04_vector_table_contract.md`
- Exception handling trong `04_kernel/03_exception_model.md`

---

## 12. Ghi chú thiết kế

- Interrupt model phải giữ được tính **đơn giản** và **dự đoán được**.
- Mọi mở rộng interrupt handling phải bắt đầu từ việc **cập nhật model**, không phải từ code.
- Interrupt **không được** trở thành cơ chế "vá" cho thiết kế kernel.

---

## 13. Implementation Notes

Interrupt model này được triển khai chi tiết trong:
- **`04_kernel/05_interrupt_infrastructure.md`** - Implementation guide cho IRQ dispatch mechanism

**Diagrams:**
- **`docs/04_kernel/diagram/irq_architecture_overview.mmd`** - High-level component architecture
- **`docs/04_kernel/diagram/irq_dispatch_contract.mmd`** - Dispatch contract và decision flow
- **`docs/04_kernel/diagram/eoi_sequence.mmd`** - EOI timing sequence (CRITICAL)

**Implementation status:**
- [ ] IRQ dispatch framework
- [ ] INTC driver
- [ ] IRQ entry stub (assembly)
- [ ] Kernel IRQ enable
- [ ] Timer interrupt test

Xem `05_interrupt_infrastructure.md` để biết implementation checklist đầy đủ và testing strategy.