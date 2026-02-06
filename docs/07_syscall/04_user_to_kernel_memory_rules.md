# User to Kernel Memory Rules

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa các quy tắc khi kernel xử lý memory arguments đến từ user task trong system call, đặc biệt là user pointers.

**Mục tiêu:**
*   Đóng băng nguyên tắc: mọi dữ liệu do user cung cấp là **không tin cậy**.
*   Định nghĩa cách kernel tiếp cận pointer từ user theo hướng **fail closed**, **deterministic**, **traceable**.
*   Cung cấp một bộ quy tắc đủ chặt để kernel không tự phá mình, ngay cả khi memory mapping hiện tại vẫn còn đơn giản.
*   Tránh việc “dereference thẳng” user pointers trong kernel như một thói quen.
*   Tài liệu này không định nghĩa một security model hoàn chỉnh và không yêu cầu memory isolation kiểu per-process address space.

## 2. Phạm vi và giả định

### 2.1. Phạm vi
Áp dụng cho mọi syscall có tham số là:
*   pointer tới buffer (đọc hoặc ghi),
*   pointer tới string,
*   pointer tới struct do user điền,
*   hoặc bất kỳ vùng nhớ nào mà user kiểm soát.

### 2.2. Giả định
*   User task chạy ở **USR**, kernel xử lý syscall ở **privileged context**.
*   Hệ thống có thể đang ở trạng thái:
    *   mapping còn shared (tĩnh), hoặc
    *   mapping đã chặn một số vùng kernel tối thiểu.
    *   Không có demand paging, không có page fault recovery.

### 2.3. Ngoài phạm vi
*   Per-process address space, per-task page tables.
*   Copy-on-write, lazy mapping, page fault handlers.
*   Capability-based security model.
*   Pointer tagging hoặc hardening nâng cao.

## 3. Threat model tối thiểu (đúng mức cho RefARM-OS)

Trong RefARM-OS, user task không được giả định “tử tế”. Kernel phải giả định user có thể:
*   truyền pointer `NULL` hoặc pointer rác,
*   truyền pointer trỏ vào kernel memory,
*   truyền pointer trỏ vào vùng MMIO,
*   truyền length cực lớn để kernel overrun buffer,
*   thay đổi nội dung buffer trong lúc kernel đang đọc (nếu shared mapping),
*   cố tình gây fault để phá flow.

**Mục tiêu của rules này không phải “chặn mọi tấn công”, mà là:**
*   kernel không bị crash dễ dàng,
*   kernel không bị user kéo vào undefined behavior,
*   hành vi luôn deterministic.

## 4. Nguyên tắc nền tảng

### 4.1. User pointer is untrusted
*   Mọi pointer từ user được coi là untrusted input, tương tự syscall number và arguments.
*   Kernel không được dereference user pointer trực tiếp như một con trỏ C bình thường nếu không có bước kiểm tra hoặc policy rõ ràng.

### 4.2. Fail closed
Khi không chắc chắn về tính hợp lệ của pointer hoặc range:
*   kernel phải **fail closed**:
    *   trả lỗi (negative return), hoặc
    *   terminate task theo policy strict.
*   Kernel không được “cố đọc thử” để xem có crash không.

### 4.3. Deterministic behavior
Cùng một input invalid phải dẫn đến cùng một outcome:
*   cùng error code hoặc cùng hành vi kill,
*   không phụ thuộc vào timing hay state ngẫu nhiên.

### 4.4. Kernel authority must remain intact
Xử lý user pointers không được:
*   làm hỏng scheduler invariants,
*   làm hỏng interrupt infrastructure,
*   hoặc tạo ra path không traceable (ví dụ fault loop không rõ nguồn).

## 5. Canonical rules for user memory arguments

RefARM-OS định nghĩa 3 lớp quy tắc, áp dụng theo thứ tự.

### 5.1. Rule 1: Range validation (address-level)
*   Kernel phải kiểm tra pointer có nằm trong vùng “được phép coi là user-accessible” không.
*   Trong giai đoạn mapping đơn giản, kernel phải công bố một hoặc nhiều vùng được coi là user memory, ví dụ:
    *   user stack region(s)
    *   user heap region (nếu có)
    *   user code/rodata region(s) (cho static strings)
*   **Implementation Update**: `validate_user_pointer` whitelist cả vùng `_user_stack` và `_text` (nơi chứa `.rodata`).
*   Vùng kernel data/stack (ngoài whitelist) phải nằm ngoài các vùng này.
*   **Contract yêu cầu**: Mỗi syscall handler khi nhận pointer phải gọi một primitive kiểm tra hợp lệ:
    *   `user_range_ok(ptr, len, access_type)`
*   Primitive này dùng các hằng số vùng nhớ được chốt trong memory map documentation.
*   Nếu mapping hiện tại vẫn shared và chưa có permission enforcement, rule này vẫn quan trọng để tránh kernel “tự đi vào vùng không intended”.

### 5.2. Rule 2: Bounds validation (length-level)
Pointer hợp lệ không đủ; length có thể làm kernel overrun. Kernel phải kiểm tra:
*   `len` không vượt max per-syscall.
*   `ptr + len` không overflow 32-bit.
*   `ptr..ptr+len` nằm trong user range.

Mỗi syscall có thể có giới hạn riêng, nhưng phải có một nguyên tắc chung:
*   Kernel không chấp nhận “unbounded copy”.
*   Ví dụ: `write(buf, len)` phải có `len <= MAX_WRITE_LEN` (giá trị cụ thể do tài liệu syscall semantics chốt).

### 5.3. Rule 3: Copy discipline (copy-in / copy-out)
Kernel không được trực tiếp thao tác trên buffer user trong thời gian dài. Thay vào đó:
*   Với data input từ user: copy vào kernel buffer (**copy-in**), sau đó xử lý.
*   Với data output về user: kernel tạo dữ liệu trong kernel buffer, sau đó copy ra user (**copy-out**).

Mục tiêu:
*   giảm bề mặt lỗi do user thay đổi buffer giữa chừng,
*   giữ kernel logic dựa trên dữ liệu ổn định,
*   làm cho lỗi pointer dễ containment hơn.

Trong scope hiện tại, kernel buffer có thể là:
*   buffer tĩnh (stack hoặc static), hoặc
*   buffer nhỏ trong syscall handler.
*   Không yêu cầu allocator phức tạp.

## 6. Access types và policy

Validation phải phân biệt “loại truy cập”:
*   **READ**: kernel đọc từ user buffer (copy-in).
*   **WRITE**: kernel ghi ra user buffer (copy-out).
*   **EXEC**: không dùng trong syscall (execute mapping thuộc memory model khác).

Trong giai đoạn hiện tại:
*   READ/WRITE là đủ.
*   Nếu MMU permissions đã bật tối thiểu: Kernel vẫn phải validate range trước khi dereference, vì permission fault có thể xảy ra và kernel phải xử lý deterministic.

## 7. Fault behavior trong quá trình copy

Ngay cả khi validate, dereference/copy vẫn có thể fault (ví dụ mapping lỗi, MMU permission, alignment).

**Kernel cần policy rõ ràng:**
*   Nếu fault xảy ra khi kernel đang copy-in/copy-out từ user:
    *   outcome phải deterministic (return error hoặc kill task).
*   Kernel không được panic chỉ vì user pointer invalid (trừ khi fault xảy ra trong privileged path không liên quan user, hoặc double fault).

**Điểm quan trọng:**
*   Kernel phải phân biệt “fault triggered by user pointer in syscall” với “kernel internal fault”.
*   Nếu kernel không thể phân biệt trong giai đoạn này, policy bảo thủ là:
    *   treat as user-caused fault nếu caller là user và fault xảy ra trong copy primitive.

## 8. Concurrency considerations (đúng mức hiện tại)

Hệ thống single-core, nhưng user buffer có thể thay đổi giữa:
*   thời điểm kernel validate,
*   và thời điểm kernel copy.

Trong giai đoạn hiện tại, RefARM-OS chấp nhận:
*   user có thể thay đổi nội dung buffer,
*   kernel không đảm bảo atomic snapshot trừ khi handler tự copy-in ngay lập tức.

**Do đó contract yêu cầu:**
*   Nếu syscall semantics yêu cầu dữ liệu nhất quán, kernel phải copy-in ngay.
*   Nếu syscall semantics chỉ cần “best effort” (ví dụ write stream), có thể đọc từng phần, nhưng vẫn phải bound và traceable.
*   Không đưa vào locking model phức tạp ở giai đoạn này.

## 9. Required kernel primitives

Để rules này có thể thực thi nhất quán, kernel cần các primitive tối thiểu:

*   `user_range_ok(ptr, len, access)`
    *   Kiểm tra pointer + len thuộc vùng user cho phép.

*   `copy_from_user(dst_k, src_u, len)`
    *   Thực hiện copy-in theo policy và có lỗi deterministic.

*   `copy_to_user(dst_u, src_k, len)`
    *   Thực hiện copy-out theo policy và có lỗi deterministic.

*   (Optional) `strnlen_user(ptr_u, max)`
    *   Dùng cho syscall nhận string, tránh đọc vượt giới hạn.

Các primitive này phải được dùng thống nhất; syscall handlers không tự viết copy logic riêng rải rác.

## 10. Observability

Khi kernel từ chối user pointer hoặc copy thất bại, kernel phải có khả năng trace tối thiểu:
*   syscall number
*   pointer value
*   length
*   access type
*   decision: `ERROR` / `KILL`

Trace không cần dump nội dung buffer.

## 11. Invariants

*   Kernel coi user pointers là untrusted input.
*   Kernel không dereference user pointers mà không có validation/bounds policy.
*   Kernel không thực hiện unbounded copy.
*   Kernel ưu tiên copy-in/copy-out để giảm phụ thuộc vào user memory trong kernel logic.
*   Pointer/copy failures không được dẫn đến undefined behavior hoặc silent corruption; outcome phải deterministic.

## 12. Explicit non-goals

Tài liệu này không nhằm:
*   cung cấp memory isolation đầy đủ như OS thương mại,
*   chống mọi tấn công,
*   hoặc xây một VM subsystem phức tạp.
*   Nó chỉ nhằm đảm bảo syscall boundary không làm kernel tự phá mình.

## 13. Quan hệ với các tài liệu khác

*   **Dựa trên:**
    *   system call scope
    *   SVC ABI contract
    *   syscall dispatch contract
    *   memory map / linker layout (để định nghĩa user ranges)

*   **Dẫn tới:**
    *   `05_minimal_syscalls.md` (các syscall cụ thể và quy tắc pointer/length từng syscall)
    *   diagrams: `user_pointer_validation.mmd`