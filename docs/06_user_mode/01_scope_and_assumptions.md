# User Mode Scope and Assumptions

## 1. Mục tiêu của tài liệu

Tài liệu này khóa phạm vi và các giả định kiến trúc cho việc đưa user-mode execution vào RefARM-OS.

**Mục tiêu:**

*   Xác định chính xác “user mode” nghĩa là gì trong RefARM-OS tại thời điểm hiện tại.
*   Đặt ranh giới rõ ràng giữa các phần: scheduler, exception/interrupt, và user-mode execution.
*   Liệt kê các giả định bắt buộc (assumptions) mà toàn bộ thiết kế user mode dựa vào.
*   Ngăn việc mở rộng vượt phạm vi (scope creep) sang system call, memory isolation đầy đủ, hoặc userland runtime.

*Lưu ý: Tài liệu này không mô tả chi tiết implementation và không phải là hướng dẫn lập trình user program.*

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi áp dụng

Phạm vi của tài liệu bắt đầu từ khi kernel đã:
*   Hoàn tất boot contract và bước vào `kernel_main()`.
*   Có interrupt infrastructure ổn định.
*   Có scheduler và context switch contract đã được khóa.
*   Có khả năng tạo và chạy nhiều task.

Trong phạm vi này, kernel bổ sung khả năng chạy một số task trong **User mode (PL0/USR)** thay vì SVC (PL1).

### 2.2. “User mode” trong RefARM-OS là gì

Trong RefARM-OS, “user mode” được hiểu theo nghĩa phần cứng:
*   CPU thực thi task ở **USR mode** (unprivileged).
*   Task không thể thực thi privileged instructions, không thể thay đổi CPSR privileged bits, không thể thao tác trực tiếp với interrupt masking, và không thể truy cập các tài nguyên chỉ cho privileged nếu MMU/permission được cấu hình để chặn.

**Mục tiêu cốt lõi:** Tách privilege boundary giữa kernel và task, để lỗi trong task không tự động trở thành lỗi phá hủy kernel logic.

### 2.3. Ranh giới trách nhiệm

*   **Execution model** định nghĩa “kernel chạy như thế nào” ở mức tổng thể.
*   **Exception model** định nghĩa “CPU/Kernel phản ứng thế nào trước sự kiện đồng bộ”.
*   **Interrupt model** định nghĩa “xử lý sự kiện bất đồng bộ”.
*   **Scheduler** định nghĩa “ai chạy tiếp theo” và “context switch contract”.

**User mode execution** không thay thế các model trên. Nó chỉ thêm một biến thể hợp lệ của “task context”: task có thể chạy ở USR thay vì SVC, với các ràng buộc tương ứng.

## 3. Assumptions (Giả định kiến trúc)

Các giả định sau là bắt buộc để user-mode execution đơn giản, traceable, và phù hợp phạm vi.

### 3.1. Single-core execution
Kernel vận hành trên môi trường single-core. Không có:
*   Cross-core scheduling.
*   Cross-core interrupt routing.
*   Concurrent execution giữa nhiều CPU.

*User-mode execution không thêm concurrency ở mức CPU, chỉ thêm ranh giới privilege.*

### 3.2. Privileged kernel, unprivileged user tasks
*   Kernel chạy ở **SVC** (privileged).
*   User tasks chạy ở **USR** (unprivileged).
*   Mọi exception/interrupt vẫn được xử lý trong privileged modes theo hành vi phần cứng ARM.
*   User tasks không có quyền can thiệp vào cơ chế exception/interrupt entry.

*Lưu ý: Hệ thống giả định chạy trong **Secure World** (TrustZone State = Secure). Kernel có toàn quyền access hardware.*

### 3.3. Shared address space (tạm thời)
Trong phạm vi hiện tại:
*   Kernel và user tasks có thể chia sẻ một không gian địa chỉ (identity map hoặc static map).
*   Mục tiêu không phải là cung cấp memory isolation đầy đủ ngay lập tức.
*   Nếu có MMU permissions, chúng chỉ nhằm bảo vệ các vùng kernel cốt lõi ở mức tối thiểu và có thể được giới hạn trong một cấu hình tĩnh đơn giản.

### 3.4. Static MMU mapping và không có dynamic memory policy
*   Mapping (nếu có) là tĩnh, được xác định trước.
*   Không có demand paging, không có page fault recovery.
*   Không có dynamic loader, không có ASLR, không có memory manager phức tạp.

*User mode execution không phụ thuộc vào cơ chế cấp phát/thu hồi bộ nhớ động.*

### 3.5. Deterministic fault handling policy
Trong phạm vi hiện tại, mọi lỗi từ user tasks được xử lý theo chính sách đơn giản và deterministic:
*   Undefined instruction, data abort, prefetch abort từ user task được coi là lỗi nghiêm trọng của task.
*   Kernel không cố gắng “khôi phục” user task theo nghĩa đầy đủ.
*   Kernel lựa chọn một trong các hành vi đơn giản: **dừng task**, **đánh dấu task chết**, hoặc **dừng hệ thống** (tùy policy đã khóa).

*Chính sách fault phải ưu tiên tính rõ ràng và traceability, không ưu tiên tính “chịu lỗi” phức tạp.*

### 3.6. No nested exceptions/interrupts as a design assumption
Trong phạm vi hiện tại:
*   Kernel không thiết kế để phụ thuộc vào nested exception/interrupt.
*   Nếu nested xảy ra ngoài ý muốn, đó là undefined behavior hoặc bị coi là bug.

*Giả định này giữ cho privilege transition và fault containment đơn giản và dễ chứng minh.*

## 4. Invariants (Bất biến kiến trúc)

Các bất biến sau phải luôn đúng nếu user-mode execution được coi là hợp lệ.

### 4.1. Kernel control is regained via hardware-defined gates only
Khi task chạy ở USR, kernel chỉ lấy lại quyền điều khiển qua:
*   Interrupt entry (IRQ).
*   Synchronous exceptions (abort/undefined).
*   Hoặc cơ chế gate được định nghĩa rõ (tương lai có thể là SVC, nhưng không giả định ở phạm vi hiện tại).

*Không có “backdoor” hoặc cơ chế nhảy trực tiếp vào kernel code từ USR.*

### 4.2. User tasks cannot change kernel scheduling authority
User task không được phép:
*   Vô hiệu hóa interrupt masking ở mức privileged.
*   Thay đổi cấu trúc scheduler.
*   Gọi trực tiếp primitive context switch của kernel bằng cách phá vỡ calling convention.

*Mọi chuyển task phải đi qua scheduler contract.*

### 4.3. Privilege boundary is enforced by hardware, not discipline
Kernel không dựa vào “task tự giác” để không phá hệ thống. Nếu một hành vi bị cấm, nó phải bị chặn bởi:
*   Privilege rules của ARM.
*   Hoặc MMU permissions tĩnh (nếu có).
*   Hoặc fault containment policy của kernel.

## 5. Explicit Non-Goals (Không nằm trong mục tiêu)

Các nội dung sau nằm ngoài phạm vi và không được giả định:
*   System call interface (SVC ABI), user-kernel API, hoặc userland runtime.
*   IPC, message passing, hoặc shared memory protocols.
*   POSIX-like process model, ELF loading, dynamic linking.
*   Memory isolation đầy đủ giữa kernel và user (per-process address space, per-task page tables).
*   User-level drivers, user-level interrupt handling.
*   Security model phức tạp (capabilities, permissions framework, sandbox policies).

*Nếu cần các mục tiêu trên, phải có tài liệu kiến trúc riêng và phải cập nhật đồng bộ execution/exception/interrupt models.*

## 6. Tiêu chí thành công

User-mode execution được coi là đạt yêu cầu khi:
1.  Có ít nhất một task chạy ở **USR** và kernel vẫn giữ quyền kiểm soát hệ thống.
2.  Lỗi do user task gây ra (undefined instruction/abort) không làm sập kernel logic, và được containment theo policy đã định.
3.  Scheduler và timer/IRQ vẫn hoạt động đúng, không bị phá vỡ bởi việc task chạy ở USR.
4.  Trace/log đủ để xác định rõ: task nào ở USR, fault xảy ra ở đâu, kernel phản ứng ra sao.

## 7. Quan hệ với các tài liệu khác

**Dựa trên:**
*   `04_kernel/01_execution_model.md`
*   `04_kernel/02_interrupt_model.md`
*   `04_kernel/03_exception_model.md`
*   `05_scheduler/03_context_switch_contract.md`

**Song hành với:**
*   `06_user_mode/02_privilege_and_cpu_mode_model.md`
*   `06_user_mode/03_mode_transition_contract.md`
*   `06_user_mode/04_fault_and_containment.md`
*   `06_user_mode/05_user_task_execution_flow.md`