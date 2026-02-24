# System Call Scope

## 1. Mục tiêu của tài liệu

Tài liệu này khóa phạm vi và các giả định kiến trúc cho system call trong RefARM-OS.

Trong RefARM-OS, system call được định nghĩa là:
*   Mục một cổng vào có kiểm soát (**controlled entry**) từ user execution (USR / unprivileged) vào kernel execution (privileged) thông qua SVC exception theo cơ chế exception của ARMv7-A.
*   Một giao thức tối thiểu để user task yêu cầu kernel thực hiện một số hành động thay mặt nó, theo một ABI cố định.

**Mục tiêu của tài liệu:**
*   Xác định rõ system call phục vụ mục đích gì trong kiến trúc RefARM-OS.
*   Thiết lập ranh giới giữa:
    *   Exception gate (SVC entry),
    *   Syscall ABI (ngôn ngữ giao tiếp),
    *   Syscall semantics (ý nghĩa từng syscall),
    *   và các subsystem liên quan (scheduler, fault containment, memory rules).
*   Ngăn “scope creep”: không để syscall trở thành “POSIX clone”, không biến userland thành runtime phức tạp.
*   Tài liệu này **không** mô tả chi tiết assembly của SVC entry, không mô tả cách cài đặt syscall table, và không bàn tối ưu hiệu năng.

## 2. Bối cảnh kiến trúc

RefARM-OS đã có các nền tảng sau:
*   Kernel mainline execution chạy ở **SVC** (privileged).
*   Một số task chạy ở **USR** (unprivileged) theo user mode model.
*   Kernel đã có mô hình exception/interrupt và contract cho vector entry/IRQ.
*   Kernel đã có scheduler và context switch contract.

Trong bối cảnh này, user mode hiện tại là “mute”: user task không có cách hợp lệ để tương tác với kernel. System call là bước đưa vào một “đường hợp lệ” để giao tiếp user–kernel.

## 3. Phạm vi

### 3.1. In-scope

Trong RefARM-OS, system call ở giai đoạn này bao gồm 5 thành phần kiến trúc:

**(1) SVC exception gate (entry boundary)**
*   User task có thể yêu cầu dịch vụ kernel chỉ bằng cách kích hoạt SVC exception.
*   Kernel tiếp nhận yêu cầu tại một entry path duy nhất, do vector table điều phối.
*   Đây là phần “cổng vào” theo cơ chế exception của ARMv7-A, không phải một function call.

**(2) Syscall ABI tối thiểu (calling convention)**
*   Quy tắc truyền:
    *   syscall number,
    *   arguments,
    *   return value,
    *   và error reporting.
*   Quy tắc clobber/preserve registers ở mức contract.
*   Quy tắc về SVC immediate (dùng hay không dùng) được chốt như một quyết định kiến trúc (không phụ thuộc “thích”).
*   ABI là “ngôn ngữ”. Không có ABI rõ ràng thì syscall chỉ là “nhảy vào kernel” một cách tùy tiện.

**(3) Syscall dispatch contract**
*   Cơ chế kernel nhận syscall request và định tuyến tới handler.
*   Ràng buộc về determinism và traceability:
    *   dispatch phải **O(1)** (table lookup),
    *   không có branch/heuristic mơ hồ,
    *   không được phép “fall-through” sang code path khác.
*   Dispatch contract phải tương thích với exception model hiện có.

**(4) User-to-kernel memory rules (tối thiểu)**
*   Ngay khi có syscall, user sẽ truyền dữ liệu, đặc biệt là pointer. RefARM-OS cần một bộ quy tắc rõ ràng:
    *   Kernel coi mọi pointer từ user là **không tin cậy**.
    *   Kernel có policy xử lý pointer: validate, bound-check, copy-in/copy-out (dù mapping còn shared).
*   Nếu giai đoạn hiện tại chưa có memory isolation đầy đủ, tài liệu vẫn phải nêu rõ: kernel dựa vào gì để không bị phá (ví dụ: giới hạn buffer, giới hạn vùng hợp lệ, và “fail closed”).
*   Quy tắc này không nhằm “bảo mật hoàn chỉnh”, mà nhằm đúng tư duy OS: user input không được phá kernel.

**(5) Minimal syscall set (small, deliberate)**
*   Syscall set chỉ cần đủ để chứng minh:
    *   user có thể tương tác với kernel qua cổng hợp lệ,
    *   kernel có thể quản lý lifecycle user task,
    *   và có I/O tối thiểu để quan sát.
*   Danh sách syscall tối thiểu sẽ được chốt ở tài liệu riêng, nhưng mục tiêu thường là:
    *   `write` (UART output)
    *   `exit` (terminate current task)
    *   (tuỳ chọn) `yield` (voluntary scheduling point)

### 3.2. Out-of-scope (explicit)

System call trong RefARM-OS không bao gồm:
*   POSIX syscall set, libc ABI đầy đủ, `errno` framework hoàn chỉnh.
*   Unix process model (fork/exec), file descriptors, VFS.
*   IPC, signals, threads, futex.
*   Dynamic linking, ELF loader, userland runtime “đúng nghĩa”.
*   Per-process address space, page tables riêng, ASLR.
*   Networking stack.
*   Debug syscall interfaces kiểu `ptrace`.

Nếu một yêu cầu rơi vào các mục trên, phải có tài liệu kiến trúc riêng và phải mở rộng scope một cách có chủ đích.

## 4. System call phục vụ cái gì trong kiến trúc RefARM-OS

System call không được xem là “tính năng” mà user đòi hỏi; nó là một bước hoàn thiện mô hình hệ điều hành ở mức kiến trúc. Có 3 mục tiêu chính:

### 4.1. Hợp thức hóa giao tiếp user–kernel
*   **Trước syscall**: User mode bị cô lập và không thể yêu cầu kernel làm gì.
*   **Sau syscall**: User có một cổng vào hợp lệ, kernel có một cổng vào kiểm soát được.
*   Điều này biến “isolation” thành “usable isolation”.

### 4.2. Cố định boundary và quyền lực của kernel
System call là nơi kernel khẳng định:
*   Kernel quyết định cái gì user được yêu cầu,
*   khi nào được yêu cầu (context rules),
*   cách yêu cầu (ABI),
*   và kernel có quyền từ chối (invalid syscall / invalid args) theo policy rõ ràng.

### 4.3. Tạo điểm quan sát và audit trung tâm
SVC entry là một “choke point” tự nhiên:
*   Mọi yêu cầu user–kernel đi qua một điểm.
*   Kernel có thể trace syscall mà không cần “instrument” khắp hệ thống.
*   Trong một OS reference, tính traceable quan trọng hơn throughput.

## 5. Assumptions (giả định kiến trúc)

### 5.1. Privilege separation đã tồn tại và là nền tảng
*   Kernel chạy privileged (SVC mainline).
*   User task chạy unprivileged (USR).
*   System call không thay thế containment policy: user vẫn có thể fault, kernel vẫn containment; syscall chỉ cung cấp đường hợp lệ để user yêu cầu dịch vụ.

### 5.2. Single-core
*   Không có concurrent entry từ nhiều CPU.
*   Không cần SMP locking model.

### 5.3. Exception model là đường vào duy nhất
*   Kernel entry từ user phải đi qua cơ chế exception của ARMv7-A.
*   SVC entry phải tuân theo layering: `vector → entry stub → normalized context → C handler`.

### 5.4. Static design, no hidden policy
*   Syscall numbering và ABI phải cố định.
*   Syscall dispatch là table-driven, không dựa vào magic.
*   Không có dynamic registration phức tạp trong giai đoạn này (trừ khi có lý do kiến trúc rõ ràng).

### 5.5. No nested exceptions as a feature
*   Syscall handling không dựa vào nested exception.
*   Nếu nested xảy ra do bug hoặc do policy chưa chốt, đó là lỗi kiến trúc cần xử lý rõ (panic/contain), không “tự nhiên chấp nhận”.

## 6. Invariants (bất biến bắt buộc)

Các bất biến dưới đây phải luôn đúng khi syscall được coi là “đúng kiến trúc”:

### 6.1. SVC là cổng hợp lệ duy nhất cho user→kernel communication
*   User không gọi kernel function trực tiếp.
*   User không nhảy vào kernel text/data.
*   Bất kỳ attempt nào ngoài SVC gate là “hostile behavior” và phải bị chặn bởi hardware/memory rules hoặc dẫn tới fault containment.

### 6.2. Syscall request luôn bị kernel coi là không tin cậy
*   Syscall number phải được kiểm tra hợp lệ.
*   Args phải được kiểm tra theo contract (range, alignment nếu cần, pointer rules).
*   Syscall handler không được giả định input “đúng”.

### 6.3. Syscall handling không được làm mất quyền lực scheduler/interrupt của kernel
User không được phép dùng syscall để:
*   thay đổi interrupt masking privileged,
*   phá vỡ scheduler invariants,
*   can thiệp vào kernel internal state không thuộc syscall semantics.

### 6.4. Return path là duy nhất và rõ ràng
*   Syscall path phải kết thúc bằng exception return (theo semantics của ARM), quay lại user tại đúng điểm.
*   Không được có “fall-through” sang code path khác.
*   Không được có “implicit continuation” khó trace.

### 6.5. Syscall path phải giữ nguyên contract của exception handling
*   Không dùng user stack cho privileged bookkeeping.
*   Context save/restore deterministic theo một layout được đóng băng.
*   Nếu syscall handler quyết định reschedule, điểm reschedule phải tuân theo scheduler contract (không phá các invariant Phase 1).

## 7. Error model (tối thiểu)

System call cần một error model rõ ràng và tối thiểu:
*   Syscall invalid (unknown number) phải có hành vi deterministic:
    *   hoặc trả lỗi (ví dụ negative return),
    *   hoặc kill task theo policy (nếu muốn strict).
*   Argument invalid phải có hành vi deterministic, tương tự.
*   Kernel không cố gắng “tự sửa” input của user.
*   Chi tiết error encoding thuộc phạm vi tài liệu ABI, nhưng scope này chốt nguyên tắc: **fail closed, deterministic**.

## 8. Success criteria

System call được coi là đạt yêu cầu khi:
*   User task có thể gọi ít nhất một syscall hợp lệ (ví dụ `write`) và kernel xử lý đúng.
*   Syscall invalid/args invalid được kernel xử lý deterministic (error hoặc kill) và traceable.
*   Syscall không phá:
    *   interrupt infrastructure,
    *   scheduler invariants,
    *   fault containment policy.
*   Trace/log đủ để audit:
    *   task nào gọi syscall gì,
    *   args mức tối thiểu (không cần dump buffer),
    *   kết quả ra sao.

## 9. Quan hệ với các tài liệu khác

System call scope này:

*   **Dựa trên:**
    *   execution model,
    *   exception model (SVC gate),
    *   privilege & cpu mode model,
    *   mode transition contract,
    *   fault and containment policy,
    *   scheduler context switch contract.

*   **Dẫn tới:**
    *   `02_svc_abi_contract.md` (ABI: numbering/args/return/clobber rules)
    *   `03_syscall_dispatch_contract.md` (dispatch invariants, table, tracing)
    *   `04_user_to_kernel_memory_rules.md` (user pointers, copy rules, bounds)
    *   `05_minimal_syscalls.md` (semantics từng syscall)

Mọi thay đổi phạm vi hoặc bất biến trong tài liệu này phải được phản ánh đồng bộ trong các tài liệu liên quan.