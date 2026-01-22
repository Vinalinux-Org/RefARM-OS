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

Boot được chia thành ba phần rõ ràng, mỗi phần tương ứng với một file riêng biệt:

1. `reset.S`  
   Xử lý sự kiện reset của CPU.

2. `vector_table.S`  
   Định nghĩa bảng vector exception.

3. `entry.S`  
   Chuẩn bị môi trường để chuyển sang C code.

Việc phân tách này nhằm:

- Làm rõ vai trò kiến trúc của từng giai đoạn.
- Tránh gộp logic không liên quan vào cùng một file.
- Giữ boot code đơn giản, dễ đọc và dễ kiểm soát.

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
|       reset.S       |
|---------------------|
| - mask IRQ/FIQ      |
| - ensure SVC mode   |
| - setup SVC stack   |
| - set vector base   |
+---------------------+
          |
          v
+---------------------+
|   vector_table.S    |
|---------------------|
| - exception vectors |
+---------------------+
          |
          v
+---------------------+
|       entry.S       |
|---------------------|
| - setup stacks      |
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

`reset.S` là đoạn code **đầu tiên** được thực thi khi CPU reset.

Nhiệm vụ của `reset.S`:

1. Mask toàn bộ IRQ và FIQ để tránh interrupt ngoài ý muốn.
2. Đảm bảo CPU đang ở SVC mode.
3. Thiết lập stack pointer ban đầu cho SVC mode.
4. Thiết lập địa chỉ vector table (low hoặc high vector theo thiết kế).
5. Chuyển điều khiển sang `entry.S`.

Các công việc **không** được thực hiện trong `reset.S`:

- Clear `.bss`.
- Gọi C code.
- Khởi tạo thiết bị (UART, timer, v.v.).

---

### 5.2. `vector_table.S`

`vector_table.S` chỉ chứa bảng vector exception của CPU.

Đặc điểm:

- Mỗi vector chỉ thực hiện `branch` đến handler tương ứng.
- Không chứa logic xử lý exception.
- Được đặt tại địa chỉ phù hợp với cấu hình vector base.

Các vector tối thiểu bao gồm:

1. Reset
2. Undefined instruction
3. SVC
4. Prefetch abort
5. Data abort
6. IRQ
7. FIQ

Mục tiêu của file này là **phản ánh đúng mô hình exception của ARM**, không xử lý logic tại đây.

---

### 5.3. `entry.S`

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
