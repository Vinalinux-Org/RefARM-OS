# Linker Layout (Boot Stage)

## 1. Mục tiêu của linker script trong giai đoạn boot

Linker script trong giai đoạn boot có nhiệm vụ xác định rõ:
- Các vùng memory được sử dụng.
- Thứ tự và vị trí của các section quan trọng.
- Địa chỉ đặt vector table, code, data, BSS và stack.

Linker script là nguồn sự thật duy nhất về memory layout.  
Mọi giả định trong boot code phải khớp tuyệt đối với linker script.

---

## 2. Giả định phần cứng và memory

Trong phạm vi OS reference này, giả định memory như sau:

- Bootloader đã load kernel image vào DDR.
- Kernel chạy trực tiếp từ DDR.
- Không sử dụng SRAM nội cho kernel chính.
- MMU tắt trong giai đoạn boot.

Memory (đơn giản hóa):

```text
+-------------------------+
|       On-chip ROM       |  <-- Boot ROM (Không đụng tới)
+-------------------------+
|      On-chip SRAM       |  <-- (Không sử dụng)
+-------------------------+
|                         |
|         DDR RAM         |  <-- Kernel Image, Stack, BSS
|                         |
+-------------------------+
```


Địa chỉ cụ thể của DDR sẽ được cấu hình theo AM335x, nhưng layout logic phải độc lập với giá trị tuyệt đối.

---

## 3. Nguyên tắc layout

Linker layout trong giai đoạn boot tuân theo các nguyên tắc sau:

1. Vector table phải nằm ở địa chỉ mà CPU có thể truy cập ngay sau reset.
2. Code và data phải nằm trong cùng một không gian liên tục.
3. BSS phải được xác định rõ ràng để có thể clear thủ công.
4. Stack phải nằm ở vùng memory an toàn, không đè lên code hoặc data.
5. Không có allocation động trong giai đoạn boot.

---

## 4. Các section chính

Kernel image trong giai đoạn boot bao gồm các section sau:

- `.vectors` : bảng vector exception.
- `.text`    : code thực thi.
- `.rodata`  : dữ liệu chỉ đọc.
- `.data`    : dữ liệu đã khởi tạo.
- `.bss`     : dữ liệu chưa khởi tạo.
- `.stack`   : vùng stack cho các CPU mode.

---

## 5. Layout tổng thể (logic)

Layout logic trong DDR được tổ chức như sau:

```text
      Low Address
           |
+----------------------------+
|          .vectors          |
+----------------------------+
|           .text            |
+----------------------------+
|          .rodata           |
+----------------------------+
|           .data            |
+----------------------------+
|           .bss             |
+----------------------------+
|                            |
|     kernel free space      |
|                            |
+----------------------------+
|           stacks           |
|  - SVC stack               |
|  - IRQ stack               |
|  - (other modes if needed) |
+----------------------------+
           |
      High Address
```


Stack được đặt ở cuối vùng kernel image và tăng trưởng xuống (descending stack).

---

## 6. Vector table placement

Vector table phải được:
- Đặt tại một địa chỉ cố định.
- Được align theo yêu cầu của ARM.

Trong giai đoạn boot:
- Sử dụng low vector (0x00000000) hoặc
- Remap vector sang địa chỉ khác nếu phần cứng cho phép.

Việc đặt vector table được kiểm soát hoàn toàn bởi linker script thông qua section `.vectors`.

---

## 7. Stack layout

Mỗi CPU mode sử dụng một stack riêng biệt để tránh ghi đè trạng thái.

Tối thiểu cần:
- SVC stack: dùng cho kernel chính.
- IRQ stack: dùng khi xử lý interrupt.

Layout stack logic:

```text
      High Address
           |
+----------------------------+
|         IRQ stack          |
+----------------------------+
|         SVC stack          |
+----------------------------+
           |
      Low Address
```


Địa chỉ stack pointer ban đầu cho từng mode được lấy từ symbol do linker export.

---

## 8. Symbol được export bởi linker

Linker script phải export các symbol sau để boot code sử dụng:

- `_vector_start` / `_vector_end`
- `_text_start` / `_text_end`
- `_bss_start` / `_bss_end`
- `_stack_top`
- `_svc_stack_top`
- `_irq_stack_top`

Các symbol này được sử dụng trong:
- `reset.S`
- `entry.S`
- Code clear BSS
- Setup stack pointer

---

## 9. Quan hệ giữa linker và boot code

Boot code không được:
- Giả định hard-code địa chỉ memory.
- Tự tính toán vị trí section.

Mọi địa chỉ quan trọng phải:
- Được lấy từ symbol do linker cung cấp.
- Được mô tả rõ trong tài liệu này.

Nếu cần thay đổi layout:
- Sửa linker script trước.
- Sau đó cập nhật boot code cho phù hợp.

---

## 10. Tiêu chí hoàn thành linker layout

Linker layout được coi là hoàn thành khi:

- Tất cả section cần thiết cho boot đều có vị trí rõ ràng.
- Stack không chồng lấn với code hoặc data.
- Boot code có thể sử dụng linker symbol mà không cần giả định ngầm.
- Layout đủ đơn giản để dễ phân tích và giải thích.

Sau khi layout này được đóng băng, mới bắt đầu viết code thực thi trong `reset.S` và `entry.S`.
