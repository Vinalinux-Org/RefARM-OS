# PROJECT_SCOPE

## 1. Mục tiêu dự án

Dự án này nhằm xây dựng một Operating System (OS) reference tối giản trên kiến trúc ARM, chạy trực tiếp trên board thật (BeagleBone Black – AM335x, Cortex-A8).

OS này không nhằm mục đích sử dụng thương mại, không tối ưu hiệu năng, không cạnh tranh với Linux hay RTOS hiện có.

Mục tiêu chính của dự án là:
- Làm rõ tư duy và quy trình thiết kế một OS từ góc nhìn kỹ sư hệ điều hành.
- Cung cấp một nền tảng reference để kỹ sư có thể học, phân tích và tự triển khai lại OS từ đầu trên chip ARM.
- Làm cơ sở kỹ thuật cho các dự án phát triển chip trong tương lai.

Sản phẩm bàn giao bao gồm code và tài liệu kỹ thuật chi tiết, trong đó tài liệu đóng vai trò ngang bằng với code.

---

## 2. Đối tượng sử dụng

Đối tượng đọc và sử dụng chính của dự án là:
- Kỹ sư viết hệ điều hành (OS engineer).
- Kỹ sư làm việc trực tiếp với chip, SoC, hoặc BSP ở mức thấp.

Dự án không hướng tới người mới học lập trình và không đơn giản hóa thuật ngữ kỹ thuật.

---

## 3. Phạm vi kỹ thuật

### 3.1. Phần cứng và kiến trúc
- Kiến trúc CPU: ARMv7-A.
- Core: Cortex-A8.
- Board mục tiêu: BeagleBone Black (AM335x).
- Chạy trực tiếp trên board thật, không ưu tiên giả lập.

### 3.2. Mô hình hệ điều hành
- OS kernel dạng monolithic, tối giản.
- Single-core, không hỗ trợ multi-core hoặc multi-chip.
- Kiến trúc được thiết kế rõ ràng, có thể mở rộng nhưng không triển khai vượt phạm vi reference.

### 3.3. Không gian thực thi
- Kernel chạy ở privileged mode.
- Có phân biệt kernel mode và user mode ở mức cơ bản.
- Không triển khai cơ chế bảo mật hoặc isolation phức tạp như OS thương mại.

---

## 4. Phạm vi chức năng

OS reference được coi là hoàn thành khi đạt được các mốc sau:

- Boot thành công từ reset vector trên board thật.
- Thiết lập trạng thái CPU ban đầu (mode, stack, vector table).
- Khởi tạo UART và xuất log sớm để debug.
- Thiết lập timer và xử lý interrupt cơ bản.
- Thiết lập MMU với mapping tĩnh, đơn giản.
- Triển khai cơ chế xử lý exception và IRQ.
- Có scheduler đơn giản (ví dụ round-robin).
- Thực hiện context switch giữa nhiều task.

Ngoài các chức năng kernel cốt lõi, OS có các thành phần mức cao tối giản:

- Cơ chế system call cơ bản để chuyển từ user mode sang kernel mode.
- Một tập system call tối thiểu (ví dụ: read, write, open, close, exit).
- Shell dòng lệnh đơn giản chạy trên UART.
- Hệ thống file ở mức đơn giản (có thể là in-memory hoặc block-based tối giản).

Các thành phần này chỉ nhằm minh họa kiến trúc OS, không nhằm đạt tính đầy đủ hay tương thích POSIX.

---

## 5. Những gì không nằm trong phạm vi

Dự án này không triển khai các nội dung sau:

- POSIX compatibility đầy đủ.
- Networking stack.
- Filesystem phức tạp hoặc journaling.
- Dynamic linking hoặc user program phức tạp.
- SMP hoặc multi-core.
- Power management nâng cao.
- Security model phức tạp.

Các nội dung trên có thể được đề cập ở mức khái niệm trong tài liệu, nhưng không được hiện thực hóa trong code.

---

## 6. Nguyên tắc phát triển

- Code được viết để minh họa tư duy và kiến trúc, không nhằm tối ưu hiệu năng.
- Mỗi module và mỗi file tồn tại phải có lý do kỹ thuật rõ ràng.
- Không trừu tượng hóa sớm khi chưa cần thiết.
- Ưu tiên tính rõ ràng, khả năng trace và khả năng giải thích.
- System call, shell và filesystem được triển khai ở mức tối thiểu cần thiết để thể hiện kiến trúc.
- Mọi phần code quan trọng phải có tài liệu kỹ thuật đi kèm.

---

## 7. Vai trò của tài liệu

Tài liệu là một phần không tách rời của sản phẩm.

Mỗi subsystem trong OS phải có:
- Mô tả kiến trúc.
- Luồng xử lý.
- Diagram minh họa.
- Giải thích quyết định thiết kế chính.

Một kỹ sư khác, chỉ dựa trên tài liệu và source code, phải có khả năng hiểu và tự triển khai lại OS này từ đầu.

---

## 8. Tiêu chí đánh giá thành công

Dự án được coi là thành công khi:

- OS boot và chạy ổn định trên board thật.
- Các cơ chế cốt lõi của OS (boot, exception, scheduler, MMU, syscall) được thể hiện rõ ràng.
- Có shell và filesystem tối giản để tương tác và kiểm chứng OS.
- Code và tài liệu đủ rõ để một kỹ sư OS khác có thể học và tái tạo lại.
- Dự án giữ được phạm vi gọn, không phát triển lan man ngoài mục tiêu ban đầu.
