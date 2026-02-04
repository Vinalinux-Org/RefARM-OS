# User Task Execution Flow

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả execution flow của một user task trong RefARM-OS: từ lúc kernel quyết định dispatch task ở USR, cho đến khi task tiếp tục chạy, bị preempt bởi IRQ, hoặc bị containment bởi fault.

**Mục tiêu:**
*   Đóng băng state machine tối thiểu của user task trong hệ thống.
*   Liên kết mode transition contract với scheduler authority và fault containment policy.
*   Làm rõ các điểm kernel “giành lại quyền điều khiển” khi user task đang chạy.
*   Đảm bảo flow traceable, deterministic, và không phụ thuộc vào syscall.

*Lưu ý: Tài liệu này không phải hướng dẫn viết user program và không mô tả ABI system call.*

## 2. Phạm vi và giả định

### 2.1. Phạm vi
Áp dụng cho các task được đánh dấu chạy ở **USR mode**.

### 2.2. Giả định
*   Single-core.
*   Kernel mainline ở SVC.
*   IRQ có thể enabled trong steady execution.
*   User task không có cơ chế gọi kernel hợp lệ (no syscall).
*   Fault từ user được xử lý theo policy containment đã khóa (khuyến nghị: **kill task**).

## 3. User task lifecycle states (mô hình tối thiểu)

Trong phạm vi hiện tại, user task có thể được mô tả bằng các trạng thái logic:
*   **READY:** task đã được tạo, có context hợp lệ, sẵn sàng được chạy.
*   **RUNNING_USR:** CPU đang thực thi task ở USR.
*   **STOPPED/DEAD:** task đã bị kill bởi containment policy hoặc bị loại khỏi scheduling.
*   *(Optional) BLOCKED: không sử dụng trong giai đoạn này.*

*Trạng thái này là mô hình logic của scheduler/task subsystem, không phải trạng thái phần cứng.*

## 4. Execution flow tổng quát

Một user task tiến triển theo vòng đời sau:
1.  Kernel chọn task tiếp theo theo scheduler policy.
2.  Kernel thiết lập CPU state để enter USR và nhảy vào entry point của task.
3.  Task chạy trong USR cho đến khi:
    *   IRQ xảy ra (asynchronous) và kernel lấy lại control tạm thời.
    *   Hoặc Fault xảy ra (synchronous) và kernel containment task.
    *   Hoặc Task chạy vô hạn (expected).

## 5. Detailed flows

### 5.1. Dispatch: Kernel schedules a user task
Điểm bắt đầu là khi `next task = user task`.
*   Kernel thực hiện: cập nhật scheduler state, thiết lập user stack và user entry PC, chuyển CPU sang USR.
*   **Kết quả:** CPU rời kernel mainline (SVC) và bắt đầu chạy user task (USR).

### 5.2. Flow A: IRQ occurs while running user task
1.  CPU switch sang **IRQ mode**.
2.  Exception entry chuẩn hoá context (không dùng user stack).
3.  Kernel IRQ core dispatch handler.
4.  IRQ handler hoàn tất (EOI, barrier).
5.  Kernel quyết định hành vi:
    *   **A1. Return to same user task (Resume):** Nếu chỉ là event, kernel return về USR, task chạy tiếp.
    *   **A2. Reschedule to another task:** Nếu IRQ tick trigger scheduler, kernel switch sang task khác. User task cũ về READY.

*Điểm bắt buộc: Mọi reschedule phải tuân theo scheduler contract và không được phá IRQ EOI protocol.*

### 5.3. Flow B: Synchronous fault occurs while running user task
1.  User task thực hiện hành vi không hợp lệ -> CPU vào **UND/ABT mode**.
2.  Exception entry chuẩn hoá context.
3.  Kernel phân loại fault là “fault-from-user”.
4.  Kernel áp dụng **Containment Policy (Kill):**
    *   Task bị đánh dấu `DEAD/STOPPED`.
    *   Kernel chọn task khác để tiếp tục.
    *   Kernel **không return** về user context cũ.

*Điểm bắt buộc: Không được silent resume.*

## 6. Scheduler authority trong user execution

Trong phạm vi hiện tại:
*   Kernel không thể dựa vào cooperative yield từ user.
*   Kernel chỉ có thể giành lại control thông qua IRQ hoặc fault.
*   Nếu hệ thống muốn đảm bảo fairness, **IRQ tick** phải tồn tại như cơ chế preemption tối thiểu.

*Tài liệu này không định nghĩa thuật toán scheduler, chỉ định nghĩa điểm mà scheduler có thể giành lại quyền quyết định khi current task là USR.*

## 7. Required observability

Để flow này traceable, kernel cần log/trace tối thiểu:
*   **Enter user task:** task id/name, entry address, user stack base.
*   **IRQ-from-user:** irq number, decision (resume vs reschedule).
*   **Fault-from-user:** fault type, task id/name, action (kill/halt).

## 8. Edge cases và expected behaviors

### 8.1. User task loops forever
*   Hành vi chấp nhận được.
*   Kernel giành lại control định kỳ qua IRQ.

### 8.2. User task faults immediately
*   Containment policy xử lý deterministically (Kill).
*   Không được rơi vào loop fault-return-fault.

### 8.3. No runnable tasks after kill
*   Kernel phải có fallback task (idle) luôn runnable.

## 9. Explicit non-goals
*   Exit semantics cho user task.
*   Blocking states (sleep, wait).
*   User-mode I/O.
*   Per-process memory spaces.

## 10. Quan hệ với các tài liệu khác
**Dựa trên:**
*   Privilege & cpu mode model.
*   Mode transition contract.
*   Fault and containment policy.
*   Scheduler task model.

**Đi cùng diagram:**
*   `user_task_lifecycle`
*   `mode_transition`
*   `fault_isolation_flow`