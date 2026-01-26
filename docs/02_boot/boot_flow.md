# Boot Flow

## 1. Mục tiêu của giai đoạn boot

Giai đoạn boot trong dự án này được định nghĩa là khoảng thời gian từ khi CPU reset cho đến khi kernel C code (`kernel_main`) bắt đầu thực thi trong một môi trường tối thiểu nhưng hợp lệ.

Boot **không** chịu trách nhiệm cung cấp đầy đủ dịch vụ hệ điều hành.  
Mục tiêu của boot là:

1. Đưa CPU về trạng thái xác định.
2. Thiết lập môi trường tối thiểu để thực thi C code an toàn.
3. Chuẩn bị nền tảng cho quá trình kernel initialization.

Điểm kết thúc của boot là khi hàm `kernel_main()` được gọi.

---

## 2. Trạng thái CPU tại thời điểm reset

Sau reset, CPU Cortex-A8 có các đặc điểm sau:

- MMU tắt.
- Cache tắt.
- Chạy ở privileged mode (SVC).
- Vector table sử dụng địa chỉ mặc định (low vectors).
- Không có stack hợp lệ.
- Không có giả định nào về nội dung RAM.

Tại thời điểm này, **mọi trạng thái cần thiết phải được thiết lập thủ công**.

---

## 3. Phân tách các thành phần boot

Boot được chia thành hai file assembly chính, mỗi phần có vai trò kiến trúc rõ ràng:

1. **reset.S**  
   - Vector table (section `.vectors`)
   - Reset handler
   - CPU hygiene: mask IRQ/FIQ, ensure SVC mode
   - Stack setup (SVC mode)
   - VBAR setup (point to vector table)

2. **entry.S**  
   - IRQ stack setup
   - BSS clear
   - Jump to kernel_main()

Việc phân tách này nhằm:

- Tách rõ CPU initialization (reset.S) vs C runtime preparation (entry.S)
- Vector table đặt cùng reset handler vì đều nằm trong boot critical code
- Giữ boot code đơn giản, dễ trace và dễ kiểm soát
- Phản ánh đúng ARM exception model mà không tách quá nhỏ

**Lưu ý về thiết kế:**
- Vector table KHÔNG tách file riêng (không có vector_table.S)
- Reset handler và vectors cùng section `.vectors` để linker place đúng thứ tự
- Exception handlers hiện tại là stubs (infinite loops) - sẽ implement đầy đủ sau

---

## 4. Luồng boot tổng thể

### 4.1. Boot sequence (tổng quan)


```text
+---------------------+
|      CPU Reset      |
+---------------------+
          |
          v
+---------------------+
|      reset.S        |
|---------------------|
| Vector table:       |
| - reset handler     |
| - undef handler     |
| - svc handler       |
| - other vectors     |
|                     |
| Reset handler:      |
| - mask IRQ/FIQ      |
| - ensure SVC mode   |
| - setup SVC stack   |
| - set VBAR          |
| - ISB barrier       |
+---------------------+
          |
          v
+---------------------+
|       entry.S       |
|---------------------|
| - setup IRQ stack   |
| - clear .bss        |
| - call kernel_main  |
+---------------------+
          |
          v
+---------------------+
|    kernel_main()    |
+---------------------+
```

## 5. Vai trò chi tiết của từng giai đoạn

### 5.1. `reset.S`

`reset.S` chứa cả vector table và reset handler - đây là code **đầu tiên** được thực thi khi CPU reset.

**Cấu trúc file:**

1. **Vector table (section `.vectors`)**
   - Đặt tại entry point của kernel (0x80000000)
   - 8 vectors theo ARM exception model
   - Reset vector branch đến reset_handler
   - Các vectors khác hiện tại là stubs (infinite loops)

2. **Reset handler (section `.text.reset`)**
   - Xử lý CPU initialization
   - Chuẩn bị môi trường cho entry.S

**Nhiệm vụ của reset handler:**

1. Mask toàn bộ IRQ và FIQ để tránh interrupt ngoài ý muốn.
2. Đảm bảo CPU đang ở SVC mode (defensive programming).
3. Thiết lập stack pointer ban đầu cho SVC mode.
4. Thiết lập VBAR (Vector Base Address Register) trỏ đến vector table.
5. Thực thi ISB (Instruction Synchronization Barrier) sau VBAR write.
6. Chuyển điều khiển sang `entry.S`.

**Các công việc KHÔNG được thực hiện trong reset.S:**

- Clear `.bss` (thuộc entry.S).
- Gọi C code (chưa có runtime environment).
- Khởi tạo thiết bị (UART, timer, v.v.).
- Setup IRQ stack (thuộc entry.S).

**Lý do thiết kế:**

- Vector table và reset handler cùng file vì boot-critical
- Section `.vectors` được linker place đầu tiên
- Reset handler ngay sau vectors để code locality tốt
- Tách rõ CPU state setup (reset.S) vs C prep (entry.S)

---

### 5.2. `entry.S`

`entry.S` đóng vai trò cầu nối giữa assembly và C code.

Nhiệm vụ của `entry.S`:

1. Thiết lập stack pointer cho các CPU mode cần thiết (IRQ, ABT, UND nếu sử dụng).
2. Clear vùng `.bss` để đảm bảo dữ liệu toàn cục trong C ở trạng thái xác định.
3. Chuẩn bị tham số cần thiết (nếu có) cho kernel.
4. Gọi hàm `kernel_main()`.

Sau khi `kernel_main()` được gọi, **giai đoạn boot kết thúc** và kernel initialization bắt đầu.

---

## 6. Ranh giới trách nhiệm giữa boot và kernel

Boot chịu trách nhiệm:

- Trạng thái CPU ban đầu.
- Stack và memory tối thiểu.
- Vector table.
- Chuyển điều khiển sang kernel.

Kernel chịu trách nhiệm:

- Khởi tạo driver.
- Scheduler.
- MMU nâng cao.
- Interrupt handling logic.
- System call và user space.

Ranh giới này phải được giữ rõ ràng để tránh việc boot trở thành *kernel thứ hai*.

---

## 7. Ghi chú thiết kế

- Boot code phải tối giản, dễ trace và dễ đối chiếu với tài liệu kiến trúc ARM.
- Không bật MMU hoặc cache trong giai đoạn reset trừ khi có lý do kỹ thuật rõ ràng.
- Mọi giả định về memory layout phải được phản ánh trong linker script.
- Boot code phải hoạt động ổn định trên board thật trước khi xem xét giả lập.

---

## 8. Tiêu chí hoàn thành giai đoạn boot

Giai đoạn boot được coi là hoàn thành khi:

1. CPU reset và chạy đúng luồng đã mô tả.
2. Stack hợp lệ cho kernel C code.
3. `kernel_main()` được gọi thành công.
4. Có thể mở rộng boot mà không phá vỡ ranh giới trách nhiệm.