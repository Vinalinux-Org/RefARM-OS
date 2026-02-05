# Mode Transition Contract

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa contract cho việc chuyển đổi CPU mode giữa:
*   Kernel execution (SVC, privileged).
*   User task execution (USR, unprivileged).

**Mục tiêu:**
*   Đóng băng các quy tắc “được phép / không được phép” khi kernel đưa CPU vào USR.
*   Định nghĩa rõ đường vào kernel từ USR (IRQ và synchronous faults).
*   Định nghĩa môi trường tối thiểu mà kernel phải thiết lập để user code chạy được mà không phá kernel.
*   Đảm bảo mọi transition đều traceable, deterministic, và không dựa vào “task tự giác”.

*Lưu ý: Tài liệu này không mô tả chi tiết instruction sequence trong assembly. Nó là contract mà implementation phải tuân theo.*

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi áp dụng
Contract này áp dụng cho các tình huống:
*   Kernel chủ động “dispatch” một task để chạy ở USR.
*   CPU đang chạy USR và một sự kiện xảy ra khiến kernel lấy lại quyền điều khiển:
    *   IRQ
    *   Undefined instruction
    *   Prefetch abort
    *   Data abort

*Trong phạm vi hiện tại, user task là “mute”: không có giao diện gọi dịch vụ kernel một cách hợp lệ (không syscall ABI).*

### 2.2. Ngoài phạm vi
*   System call ABI (SVC như một interface có định danh, arguments, return values).
*   User-to-kernel communication protocol.
*   Nested interrupts/exceptions như một tính năng.
*   Per-task address space hoặc page tables riêng.

## 3. Định nghĩa transition events

User-mode execution có hai nhóm transition events:

### 3.1. Voluntary transitions (Supported: Yield Only)
*   User code chủ động tạo ra exception để quay về kernel thông qua `SVC #0`.
*   Đây là cơ chế primitive cho **Voluntary Yield**.
*   Các syscall khác chưa được hỗ trợ.

### 3.2. Involuntary transitions (được sử dụng)
Kernel lấy lại quyền điều khiển từ user bằng các gate do phần cứng định nghĩa:
*   **IRQ** (asynchronous).
*   **Undefined instruction** (synchronous).
*   **Prefetch abort / Data abort** (synchronous).

*Những gate này là cơ sở cho “containment”: user có thể sai, nhưng kernel vẫn lấy lại control theo đường chuẩn.*

## 4. Contract: Kernel → User (enter USR execution)

### 4.1. Preconditions
Trước khi kernel chuyển CPU sang USR để chạy user task, các điều kiện sau phải đúng:
1.  **Vector base (VBAR)** đã được cấu hình và exception entry paths đã sẵn sàng.
2.  **Stack cho các exception modes** cần thiết đã được thiết lập tối thiểu: SVC stack, IRQ stack, ABT stack, UND stack.
3.  **Scheduler state** hợp lệ và kernel có thể xác định “current task” và “next task”.
4.  **IRQ policy** rõ ràng:
    *   IRQ có thể enabled để hệ thống tiếp tục “có nhịp”.
    *   Hoặc IRQ masked nếu đang ở bước bring-up (nhưng khi đó user task không được coi là execution target lâu dài).

### 4.2. State setup requirements
Kernel phải chuẩn bị đủ CPU state để USR code chạy mà không đụng vào privileged state.

**Yêu cầu tối thiểu:**
*   Một **user stack** hợp lệ (`SP_usr`) phải được set tới vùng stack của task.
    *   **CRITICAL:** Stack Pointer phải được align **8-byte** (theo chuẩn AAPCS) tại boundary user entry.
*   **User entry PC** phải được set đúng (địa chỉ entry của user task).
*   **CPSR** khi vào user phải phản ánh:
    *   Mode = USR (`0x10`).
    *   Interrupt Mask (`I` bit) = 0 (IRQ Unmasked/Enabled).
    *   Mode = USR (`0x10`).
    *   Interrupt Mask (`I` bit) = 0 (IRQ Unmasked/Enabled).
    *   Thumb Bit (`T` bit) = Phải khớp với instruction set của User Code (0=ARM, 1=Thumb). *Nếu sai lệch, CPU sẽ crash ngay lập tức.*

**Cơ chế "Artificial Exception Return":**
Để start user task lần đầu, Kernel đang ở SVC mode phải "giả vờ" rằng nó vừa bị ngắt từ User mode.
1.  Kernel ghi `0x10` (USR) vào `SPSR_svc`.
2.  Kernel ghi User Entry Point vào `LR_svc`.
3.  Kernel thực thi instruction đặc biệt: `MOVS PC, LR`.
    *   Instruction này copy `LR` -> `PC`.
    *   Đồng thời copy `SPSR` -> `CPSR` (vì có hậu tố `S` và đích là `PC`).
    *   Kết quả: CPU nhảy tới User Code và switch sang mode USR trong cùng 1 cycle nguyên tử.

*Kernel không được dựa vào “user code sẽ không dùng stack” hoặc “user code sẽ không fault”.*

### 4.3. Postconditions
Sau khi enter user:
*   CPU thực thi tại user entry point trong USR mode.
*   Kernel không còn quyền điều khiển cho đến khi một involuntary transition xảy ra (IRQ/fault).
*   Không có bất kỳ giả định nào rằng user sẽ quay lại kernel “một cách đẹp”.

## 5. Contract: User → Kernel (kernel regains control)

### 5.1. Entry gates
Kernel chỉ được lấy lại quyền điều khiển từ user qua:
*   IRQ entry.
*   ABT entry (data/prefetch abort).
*   UND entry.

*Không có đường “call kernel function” trực tiếp từ user.*

### 5.2. Exception entry invariants
Khi CPU chuyển từ USR sang một exception mode, exception entry layer phải đảm bảo:
*   Không sử dụng user stack để lưu context privileged.
*   Save/restore context phải deterministic và độc lập với “user program correctness”.
*   Kernel handler được gọi trong một môi trường đã chuẩn hoá.

**Tối thiểu phải preserve:**
*   General-purpose registers r0–r12.
*   User SP/LR (r13_usr, r14_usr) nếu cần phục hồi user execution chính xác.
*   Return state (`SPSR_<mode>`) để kernel có thể quyết định return hoặc containment.
*   **STACK ALIGNMENT (AAPCS):** Exception stack (IRQ/SVC/ABT) phải giữ alignment **8-byte** trước khi gọi bất kỳ hàm C nào (dispatcher). Nếu số lượng registers được push là lẻ, implementation phải push thêm padding.

### 5.3. Classification rule
Ngay khi vào kernel từ user, kernel phải phân loại nguyên nhân vào 1 trong 2 nhóm:
*   **Asynchronous event:** IRQ.
*   **Synchronous fault:** UND/ABT.

*Phân loại này quyết định policy tiếp theo (reschedule hoặc kill/contain).*

## 6. Return semantics and containment

Kernel có hai lựa chọn hợp lệ sau khi lấy lại control từ user:

### 6.1. Return to the same user task (resume)
**Điều kiện để resume:**
*   Event là IRQ (không phải fault), hoặc fault đã được xử lý theo policy.
*   User context được bảo toàn đầy đủ và hợp lệ.
*   Kernel quyết định không thay đổi “current task” (không reschedule).

**Kết quả:** Exception return quay lại USR, tiếp tục chạy tại user PC tương ứng.

### 6.2. Do not return to that user task (contain)
**Containment áp dụng khi:**
*   Task gây ra synchronous fault (UND/ABT).
*   Hoặc kernel quyết định task không còn hợp lệ để tiếp tục (policy kill).
*   Hoặc kernel cần lấy lại control lâu dài (system stability).

**Contract:** Containment phải biến lỗi task thành một hành vi deterministic của kernel:
*   Mark task as dead / stopped.
*   Chuyển execution sang kernel-selected task (idle hoặc shell kernel task).
*   **Không quay lại user task đó nữa.**

*Nếu kernel “contain” bằng cách quay lại user task mà không thay đổi gì, đó là bug (vì fault sẽ lặp vô hạn).*

## 7. Scheduling rule under user execution

Trong phạm vi hiện tại, user task không có cơ chế yield hợp lệ. Vì vậy, nếu kernel muốn hệ thống tiếp tục đa nhiệm khi user task đang chạy, scheduler phải dựa vào involuntary kernel entry để giành lại CPU.

**Contract đặt ra quy tắc:**
*   Scheduler authority luôn thuộc kernel (privileged).
*   Reschedule decision có thể được kích hoạt bởi IRQ tick.
*   Nếu reschedule xảy ra trong đường IRQ-from-user, implementation phải đảm bảo:
    *   EOI protocol được hoàn thành đúng.
    *   Không dùng user stack cho privileged bookkeeping.
    *   Không để tồn tại “dead code” giả định sẽ chạy sau context switch.

*Tài liệu thuật toán scheduler không thay đổi vì user mode; chỉ thay đổi “điểm giành lại control” khi current task là USR.*

## 8. Interrupt masking rules
*   User code không được phép điều khiển interrupt mask bits ở mức privileged.
*   Kernel phải chọn một policy rõ ràng và nhất quán:
    *   **IRQ enabled** trong steady execution.
    *   **IRQ masked** chỉ được dùng trong bring-up.

*Trong mọi trường hợp, policy phải được thực thi bởi kernel, không dựa vào hành vi user.*

## 9. Invariants (bất biến bắt buộc)
Các bất biến sau luôn phải đúng nếu mode transition được coi là hợp lệ:
1.  Kernel regains control only via hardware-defined exception gates.
2.  Privileged context never relies on user stack.
3.  Context save/restore is deterministic and complete for the chosen policy.
4.  Fault containment is explicit; fault không thể dẫn đến “quiet resume” mà không đổi state.
5.  Interrupt EOI protocol không được phép bị bỏ qua bởi bất kỳ path reschedule nào.

## 10. Explicit non-goals
Contract này không bao gồm:
*   Một syscall interface hợp lệ.
*   Một ABI cho argument passing giữa user và kernel.
*   User-kernel shared services (print, exit, sleep).
*   Memory isolation đầy đủ.
*   Nested interrupts/exceptions như một feature.

## 11. Quan hệ với các tài liệu khác
**Dựa trên:**
*   Execution model.
*   Exception model (mode switching, SPSR/LR, stacks).
*   Interrupt model (IRQ context rules).
*   Scheduler context switch contract.

**Dẫn tới:**
*   `04_fault_and_containment.md` (policy khi fault từ user).
*   `05_user_task_execution_flow.md` (state machine/lifecycle khi user task chạy).