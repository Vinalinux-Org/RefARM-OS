# Boot Contract

Tài liệu này định nghĩa “hợp đồng kỹ thuật” giữa bootloader và kernel
trong dự án OS reference cho AM335x.

Mục tiêu của boot contract là:
- Xác định rõ trách nhiệm của bootloader
- Xác định trạng thái phần cứng tại thời điểm kernel entry (`_start`)
- Xác định rõ những giả định kernel được phép và không được phép đưa ra

Boot contract này là nền tảng cho toàn bộ thiết kế boot flow
và mã khởi động của kernel.

---

## 1. Phạm vi của boot contract

1.1. Boot contract áp dụng cho thời điểm bắt đầu từ khi
kernel nhận quyền điều khiển tại entry point `_start`.

1.2. Các giai đoạn trước đó (Boot ROM, SPL, U-Boot, hoặc bootloader khác)
nằm ngoài phạm vi triển khai của kernel, nhưng ảnh hưởng trực tiếp
đến trạng thái phần cứng tại thời điểm entry.

1.3. Boot contract này không phụ thuộc vào việc bootloader
được triển khai bởi vendor hay do tự phát triển trong tương lai.

---

## 2. Trách nhiệm của bootloader

Bootloader chịu trách nhiệm đảm bảo các điều kiện sau
trước khi chuyển quyền điều khiển sang kernel:

2.1. Memory
- External DDR đã được khởi tạo và sẵn sàng sử dụng.
- Kernel image đã được load đầy đủ vào RAM tại địa chỉ xác định.
- Nội dung kernel image trong RAM là toàn vẹn.

2.2. CPU
- CPU đang chạy ở chế độ privileged.
- Không có core khác hoạt động (hệ thống single-core).

2.3. Clock và reset
- Clock tree cơ bản đã được cấu hình.
- Các clock cần thiết cho các peripheral tối thiểu đã hoạt động.

2.4. Chuyển quyền điều khiển
- Bootloader chuyển quyền điều khiển sang kernel
  thông qua entry point `_start`.
- Không có xử lý trung gian sau khi nhảy sang kernel.

---

## 3. Trạng thái hệ thống tại kernel entry

Tại thời điểm `_start` bắt đầu thực thi, kernel giả định
trạng thái hệ thống như sau:

3.1. CPU state
- CPU đang ở chế độ SVC.
- IRQ và FIQ đang bị mask.
- Không có exception đang pending.

3.2. Memory state
- MMU đang ở trạng thái tắt (MMU OFF).
- Địa chỉ vật lý trùng với địa chỉ CPU truy cập.
- Kernel đang chạy trực tiếp trên physical address space.

3.3. Cache
- Kernel không giả định cache đã được bật.
- Mọi hành vi của kernel phải đúng ngay cả khi cache đang tắt.

---

## 4. Những việc kernel KHÔNG được giả định

Để giữ boot flow đơn giản, rõ ràng và portable,
kernel không được giả định các điều kiện sau:

4.1. Không giả định bootloader cụ thể
- Kernel không phụ thuộc vào MLO, U-Boot hay bất kỳ bootloader cụ thể nào.
- Kernel không gọi hoặc tương tác lại với bootloader sau khi entry.

4.2. Không giả định hệ thống đã sẵn sàng đầy đủ
- Không giả định interrupt đã được enable.
- Không giả định timer đã được cấu hình.
- Không giả định watchdog đã bị disable.

4.3. Không giả định môi trường runtime cao cấp
- Không giả định có heap hoặc memory allocator.
- Không giả định có device tree hoặc firmware interface.

---

## 5. Trách nhiệm của kernel trong giai đoạn boot

Sau khi nhận quyền điều khiển tại `_start`,
kernel chịu trách nhiệm:

5.1. Thiết lập lại môi trường CPU tối thiểu
- Xác nhận hoặc thiết lập lại stack cho các mode cần thiết.
- Thiết lập vector base cho exception handling.

5.2. Thiết lập môi trường runtime tối thiểu
- Clear vùng `.bss`.
- Chuẩn bị môi trường để chuyển sang C code an toàn.

5.3. Không thực hiện các chức năng ngoài phạm vi boot
- Không enable interrupt trong giai đoạn boot tối thiểu.
- Không khởi tạo scheduler hoặc subsystem phức tạp.

---

## 6. Mối quan hệ với các tài liệu khác

6.1. Boot contract này là cơ sở cho:
- `boot_flow.md`
- `reset_sequence.md`
- Các sơ đồ boot trong thư mục `diagrams/`

6.2. Nếu có thay đổi về:
- Trạng thái phần cứng tại entry
- Trách nhiệm của bootloader
- Phạm vi boot của kernel

thì boot contract này phải được cập nhật trước
khi chỉnh sửa các tài liệu boot khác.

---

## 7. Ghi chú về tương lai

7.1. Khi bootloader được tự phát triển (MLO/U-Boot custom),
boot contract này vẫn được giữ nguyên trừ khi có quyết định
kiến trúc thay đổi rõ ràng.

7.2. Mọi thay đổi làm phá vỡ boot contract
phải được xem là thay đổi kiến trúc, không phải thay đổi triển khai.

---
