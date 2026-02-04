# Fault and Containment

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa mô hình fault phát sinh từ user task và cơ chế containment trong RefARM-OS.

**Mục tiêu:**
*   Xác định các loại fault quan trọng khi task chạy ở USR.
*   Làm rõ kernel phải phản ứng như thế nào để giữ kernel core ổn định và deterministic.
*   Định nghĩa các quyết định “resume / kill / halt” như policy có chủ đích, không phải hành vi ngẫu nhiên.
*   Khóa ranh giới: fault handling trong giai đoạn này phục vụ isolation proof, không phục vụ reliability hoặc user experience.

*Lưu ý: Tài liệu này không mô tả chi tiết exception entry assembly hay decoding register-level syndrome.*

## 2. Phạm vi và giả định

### 2.1. Phạm vi
Tài liệu áp dụng cho faults phát sinh khi CPU đang chạy user task (USR) và kernel lấy lại control qua **synchronous exception gates**:
*   Undefined instruction
*   Prefetch abort
*   Data abort

*IRQ không phải fault, nhưng có thể là “đường quay về kernel” và do đó có liên hệ gián tiếp với containment policy.*

### 2.2. Giả định
*   Single-core.
*   No nested exceptions/interrupts như một tính năng.
*   Address space có thể vẫn shared ở mức mapping; containment tập trung vào privilege boundary và execution control.
*   Kernel giữ quyền quyết định scheduling và task lifecycle.

## 3. Định nghĩa fault trong RefARM-OS

Trong phạm vi RefARM-OS:
*   **Fault** là sự kiện đồng bộ (synchronous) do instruction hoặc memory access của user task gây ra, khiến CPU chuyển vào exception mode.
*   Fault là biểu hiện của “user task không hợp lệ để tiếp tục” theo một policy rõ ràng của kernel.
*   Fault không được coi là cơ chế điều phối scheduling, signaling IPC hay “kênh giao tiếp ngầm”.

*Nếu user code cố tình tạo fault để yêu cầu dịch vụ kernel, đó là hành vi nằm ngoài scope (và sẽ bị containment như fault).*

## 4. Các loại fault được xem xét

### 4.1. Undefined Instruction
Xảy ra khi user task thực thi instruction không hợp lệ hoặc không được hỗ trợ.
*   **Policy:** Lỗi nghiêm trọng của task, không có recovery path hợp lệ.

### 4.2. Prefetch Abort
Xảy ra khi CPU không thể fetch instruction từ địa chỉ mà user task nhảy tới (nhảy tới địa chỉ không map, permission fault...).
*   **Policy:** Lỗi nghiêm trọng của task, **không resume**.

### 4.3. Data Abort
Xảy ra khi user task thực hiện load/store vào vùng không hợp lệ hoặc bị chặn bởi permission (pointer sai, alignment fault...).
*   **Policy:** Lỗi nghiêm trọng của task, **không resume**.

## 5. Containment policy

Containment là phản ứng có chủ đích của kernel để giữ kernel core ổn định, traceable và đảm bảo fault không lan hủy logic kernel.

### 5.1. Policy A: Kill offending task (Recommended)
Đây là policy phù hợp với mục tiêu “proof of isolation”.
*   **Hành động:**
    1.  Kernel ghi nhận fault (type + task identity).
    2.  Kernel đánh dấu task ở trạng thái **DEAD/STOPPED**.
    3.  Kernel chọn task khác để tiếp tục (idle hoặc shell kernel task).
    4.  Kernel không bao giờ return về user context của task đó.
*   **Tính chất:** Fault được containment cục bộ theo task; Kernel core tiếp tục chạy; Hành vi deterministic.

### 5.2. Policy B: Halt system (Debug fallback)
Trong một số giai đoạn bring-up, kernel có thể chọn halt để giữ hiện trường debug.
*   **Hành động:** Stop hệ thống, in diagnostics.
*   **Khi nào dùng:** Nghi ngờ kernel setup sai, cần debug sâu.

### 5.3. Policy C: Resume (Explicitly NOT supported)
*   **Hành động:** Return lại user task sau khi fault.
*   **Lý do từ chối:** Đòi hỏi phải hiểu nguyên nhân, sửa state/memory hoặc emulate instruction – những thứ nằm ngoài phạm vi.
*   **Hậu quả:** Nếu implementation “vô tình resume”, fault sẽ lặp vô hạn -> Bug containment.

## 6. Containment invariants

Containment phải tuân thủ các bất biến sau:

### 6.1. No silent resume
Không được phép thực hiện chu trình: user fault -> kernel return -> user fault -> ... (loop vô hạn không trace). Kernel phải đưa hệ thống sang một state mới rõ ràng (Kill hoặc Halt).

### 6.2. No Recursive Faults (Double Fault Panic)
Nếu trong quá trình xử lý Fault (đang ở Exception Mode) mà lại xảy ra thêm Fault nữa (ví dụ: Kernel truy cập bad pointer khi đang kill task):
*   Hệ thống coi đây là **Kernel Parsing Error** (lỗi thảm họa).
*   **Action:** Kernel Panic ngay lập tức (Halt CPU). Không được phép cố gắng "recover" từ recursive fault.

### 6.2. Fault must not corrupt kernel scheduling authority
Dù fault xảy ra, kernel vẫn phải giữ:
*   Current task pointer nhất quán.
*   Scheduler invariants (at most one RUNNING task).
*   IRQ dispatch contract (EOI đúng).

### 6.3. Fault handling is deterministic and minimal
Trong fault handler path:
*   Không allocate memory động.
*   Không gọi logic phức tạp.
*   Chỉ làm việc tối thiểu: classify → record → apply policy → handoff to scheduler.

## 7. Required observability

Containment không có giá trị nếu không quan sát được. Kernel phải log tối thiểu:
*   Task identity (name/id).
*   Fault type (UND / PABT / DABT).
*   Mode/Source classification (fault-from-user).
*   Policy action (KILL / HALT).

### 7.1. Hardware Diagnostics (Optional but Recommended)
Để hỗ trợ kỹ sư debug, Kernel nên dump giá trị các thanh ghi phần cứng khi Fault xảy ra:
*   **DFSR** (Data Fault Status Register): Cho biết nguyên nhân Data Abort (Permission, Alignment, Translation...).
*   **IFSR** (Instruction Fault Status Register): Cho biết nguyên nhân Prefetch Abort.
*   **FAR** (Fault Address Register): Địa chỉ bộ nhớ gây ra lỗi.
*   **LR_und** / **LR_abt**: Địa chỉ instruction gây lỗi (hoặc instruction kế tiếp).

*Việc đọc các register này giúp phân biệt rõ ràng giữa "Bug Pointer" (Alignment/Permission) và "System Corruption" (Translation).*

## 8. Interaction với IRQ path

IRQ không phải fault, nhưng:
*   User task có thể chạy rất lâu; kernel cần IRQ tick để giành lại control.
*   Fault handling xảy ra trong exception context; kernel phải đảm bảo IRQ protocol không bị phá vỡ.

## 9. Explicit non-goals

Tài liệu này không bao gồm:
*   Page fault recovery.
*   Copy-on-write.
*   Signal delivery hoặc user-visible fault reporting.
*   Exception unwind và recovery.
*   Fault emulation.
*   Debug exception model.

## 10. Quan hệ với các tài liệu khác

**Dựa trên:**
*   Exception model (types, layering).
*   Mode transition contract (đường vào từ user).
*   Scheduler contract (lifecycle).

**Dẫn tới:**
*   `05_user_task_execution_flow.md` (lifecycle khi task chạy, bị kill, hoặc bị thay thế).