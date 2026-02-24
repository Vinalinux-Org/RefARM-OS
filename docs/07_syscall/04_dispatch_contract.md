# Syscall Dispatch Contract

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa contract cho đường xử lý system call trong RefARM-OS: từ lúc user task thực thi SVC cho đến khi kernel trả kết quả về user (hoặc không return nếu syscall định nghĩa như vậy).

**Mục tiêu:**
*   Đóng băng cấu trúc đường đi: `vector → entry stub → normalized context → dispatch → handler → return`.
*   Đảm bảo syscall handling traceable, deterministic, và không phụ thuộc vào hành vi “tự giác” của user.
*   Ràng buộc việc syscall tương tác với scheduler và interrupt infrastructure theo các invariant đã khóa.
*   Làm rõ các trường hợp kernel không được phép thực hiện trong syscall path để tránh vi phạm execution model.
*   Tài liệu này không mô tả chi tiết code của từng syscall. Semantics của syscall nằm ở tài liệu riêng.

## 2. Phạm vi

### 2.1. In-scope
*   SVC entry path và normalized context.
*   Syscall number decode và dispatch table.
*   Quy tắc xử lý syscall hợp lệ / không hợp lệ.
*   Quy tắc tương tác với scheduler (nếu có syscall yield hoặc exit).
*   Quy tắc return path và error model.

### 2.2. Out-of-scope
*   Syscall semantics chi tiết cho từng syscall.
*   User pointer validation/copy rules (tài liệu riêng).
*   Nested exceptions như một feature.
*   Per-process address space và memory protection nâng cao.

## 3. Kiến trúc đường đi (topology)

Đường xử lý syscall là một pipeline cố định:
1.  **Vector table** nhận SVC exception.
2.  **SVC entry stub** (assembly) chuyển control vào kernel theo luật exception model và chuẩn hóa môi trường.
3.  **Normalized syscall context**: kernel có một representation nhất quán của:
    *   caller mode (USR),
    *   register snapshot,
    *   syscall number và args theo ABI.
4.  **Dispatch**: table lookup O(1) chọn syscall handler.
5.  **Syscall handler**: thực thi semantics.
6.  **Return path**:
    *   return về user (exception return),
    *   hoặc không return nếu syscall là noreturn.

Không được có đường tắt hoặc “special-case path” ngoài pipeline này.

## 4. SVC Entry Stub Contract (assembly responsibilities)

SVC entry stub phải đảm bảo các điều sau trước khi gọi vào C dispatch:

### 4.1. Mode and stack discipline
*   Khi SVC xảy ra từ user, CPU sẽ chuyển sang privileged mode để xử lý exception.
*   Entry stub phải chạy trên kernel-controlled stack (SVC stack hoặc dedicated SVC-exception stack theo thiết kế hiện có).
*   Entry stub không được dùng user stack để lưu privileged bookkeeping.

### 4.2. Context capture (minimum)
Entry stub phải capture đủ thông tin để kernel có thể:
*   đọc syscall number và args theo ABI,
*   trả kết quả về đúng user context,
*   preserve các registers theo ABI clobber/preserve rules.

**Yêu cầu tối thiểu:**
*   Snapshot các general-purpose registers `r0..r3` (args slots).
*   Snapshot `r7` (syscall number).
*   Preserve `r4..r11` (callee-saved) như một phần của task context contract.
*   Preserve user `sp` và user `lr` theo thiết kế task context (nếu kernel lưu toàn bộ user context trong task frame).
*   Preserve exception return state:
    *   `SPSR_svc` (caller CPSR),
    *   `LR_svc` (return address).
*   Entry stub không bắt buộc “đi sâu” decode nguyên nhân exception vì SVC đã là loại exception xác định.

### 4.3. Canonical syscall frame
Entry stub phải cung cấp cho C-level dispatch một “frame” có layout ổn định, ví dụ:
*   `struct syscall_frame` hoặc một stack layout cố định mà dispatch có thể truy cập.
*   Contract yêu cầu: layout này là **canonical** và không thay đổi tùy syscall.

## 5. Dispatch Contract (C-level)

### 5.1. Dispatch is table-driven and O(1)
Syscall dispatch phải là:
*   table lookup bằng syscall number,
*   không dùng string match,
*   không dùng if-else chain dài,
*   không có “auto-detect” heuristic.
*   Điều này phục vụ traceability và determinism.

### 5.2. Syscall number validation
*   Kernel phải validate syscall number (`r7`).
*   Nếu invalid:
    *   xử lý deterministic theo policy đã chốt:
        *   return error, hoặc
        *   terminate task (strict).
*   Không được silent ignore hoặc hành xử phụ thuộc trạng thái ngẫu nhiên.

### 5.3. Argument contract enforcement
*   Dispatch không được “tự sửa” arguments.
*   Dispatch chỉ truyền arguments cho handler theo ABI.
*   Nếu cần validation, handler thực hiện theo semantics hoặc theo memory rules (pointer validation).

### 5.4. Caller mode verification
Syscall được coi là hợp lệ chỉ khi caller là user (**USR**). Dispatch phải có khả năng phân biệt:
*   **SVC-from-user**: hợp lệ.
*   **SVC-from-kernel/privileged**: không hợp lệ (bug hoặc misuse).
*   Policy cho SVC-from-privileged phải deterministic (panic hoặc error). Dispatch contract yêu cầu: không coi đây là hành vi bình thường.

## 6. Syscall Handler Contract

### 6.1. Handler execution context
Syscall handler chạy trong privileged context do SVC entry cung cấp. Handler phải tuân thủ:
*   Không dùng user stack cho privileged bookkeeping.
*   Không phá task context invariants.
*   Không phá interrupt infrastructure invariants.

### 6.2. Allowed operations
Trong scope tối thiểu, handler được phép:
*   thao tác trên kernel-managed objects (task state, scheduler state) theo semantics.
*   gọi UART driver để output (ví dụ `write`).
*   đánh dấu task `DEAD` (ví dụ `exit`).
*   yêu cầu reschedule một cách có kiểm soát (ví dụ `yield`, hoặc `exit` dẫn đến schedule task khác).

### 6.3. Disallowed operations (explicit)
Trong syscall handler path, không được:
*   enable/disable IRQ một cách tùy tiện như side effect ngoài policy.
*   gọi context switch primitive ở context không phù hợp với scheduler contract.
*   thực hiện blocking wait/IPC (vì chưa có model).
*   allocate memory động nếu allocator chưa được coi là stable và contract rõ (nếu dự án đã có allocator thì phải có contract riêng).

**Nguyên tắc:** syscall handler phải nhỏ, deterministic, và dễ audit.

## 7. Interaction với Scheduler

Một số syscall sẽ tương tác trực tiếp với scheduler. Contract phải khóa rõ để tránh “switch ở sai context”.

### 7.1. Syscall yield (nếu tồn tại)
*   `yield` là voluntary scheduling point: user yêu cầu kernel nhường CPU.
*   `yield` không được phép phá scheduler invariants.
*   Sau `yield`, kernel có hai lựa chọn hợp lệ:
    *   return về user task hiện tại nếu scheduler quyết định không switch,
    *   hoặc switch sang task khác theo scheduler contract.
*   Nếu switch xảy ra, switch phải được thực hiện tại một điểm mà scheduler contract cho phép (kernel-controlled continuation point), không phải bằng việc “nhảy bừa” ra khỏi SVC handler mà chưa chuẩn hoá context.

### 7.2. Syscall exit
*   `exit` là **noreturn** từ góc nhìn user.
*   Handler phải:
    *   mark current task `DEAD`,
    *   không return về user context đó nữa,
    *   chuyển control sang scheduler để chọn task khác.
*   `exit` là trường hợp bắt buộc “do not return”.

### 7.3. Syscall không tương tác scheduler (ví dụ write)
*   `write` có thể return về user ngay.
*   `write` không được “bí mật” yield hoặc switch task trừ khi semantics ghi rõ.

## 8. Return Path Contract

### 8.1. Normal return
Nếu syscall được định nghĩa là returning:
*   Kernel đặt return value vào `r0` theo ABI.
*   Kernel thực hiện exception return để quay lại USR tại instruction sau SVC.
*   Registers preservation/clobber tuân theo ABI:
    *   `r0..r3`, `r7` clobbered,
    *   `r4..r11`, `sp`, `lr` preserved.

### 8.2. Noreturn return (exit)
Nếu syscall định nghĩa noreturn:
*   Kernel không được exception-return về user.
*   Kernel phải đảm bảo execution tiếp tục ở task khác hoặc ở kernel idle path.
*   Không được để tồn tại “dangling” user context quay lại.

### 8.3. Error return
Nếu syscall invalid hoặc argument invalid theo policy “return error”:
*   Kernel trả error code âm trong `r0`.
*   Kernel return về user như normal return.
*   Nếu policy “kill task”, thì đây là noreturn đối với task đó.

## 9. Observability and Tracing Contract

Dispatch layer phải có điểm log/trace tối thiểu:
*   syscall entry:
    *   task id/name
    *   syscall number
    *   args `r0..r3`
*   syscall exit:
    *   return value `r0`
    *   outcome: `OK` / `ERROR` / `KILL`

Trace phải nằm ở một nơi tập trung (dispatch), không rải rác ở từng handler.

## 10. Invariants (bất biến bắt buộc)

*   Syscall entry luôn đi qua SVC vector và entry stub chuẩn hóa context.
*   Syscall dispatch là table-driven O(1), không heuristic.
*   Syscall number và args tuân theo ABI, được validate.
*   Syscall handler không phá scheduler/interrupt invariants.
*   Return path là explicit và đúng semantics:
    *   returning syscall: exception return về USR,
    *   noreturn syscall: không quay lại user context đó.
*   User stack không được dùng cho privileged bookkeeping trong syscall path.
*   Syscall tracing có entry/exit points tập trung.

## 11. Explicit non-goals
Dispatch contract này không bao gồm:
*   pointer validation/copy rules chi tiết,
*   semantics của từng syscall,
*   nested exceptions support,
*   re-entrancy/locking model SMP.

## 12. Quan hệ với các tài liệu khác

*   **Dựa trên:**
    *   `07_syscall/02_svc_abi_contract.md`
    *   exception model (SVC entry semantics)
    *   scheduler context switch contract
    *   privilege/mode transition model

*   **Dẫn tới:**
    *   `07_syscall/04_user_to_kernel_memory_rules.md`
    *   `07_syscall/05_minimal_syscalls.md`
    *   diagrams: `svc_entry_flow`, `syscall_dispatch`, `syscall_return_path`