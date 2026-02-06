# Minimal Syscalls

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa tập system call tối thiểu của RefARM-OS, bao gồm:
*   danh sách syscall IDs (numbering),
*   semantics (ý nghĩa hành vi),
*   quy tắc arguments/return/error,
*   và các ràng buộc về memory (pointer/length) theo `04_user_to_kernel_memory_rules.md`.

**Tập syscall này được thiết kế để:**
*   chứng minh user–kernel communication qua SVC gate hoạt động đúng,
*   cung cấp I/O tối thiểu để quan sát hệ thống,
*   và quản lý lifecycle user task ở mức tối thiểu.
*   Tài liệu không nhằm cung cấp POSIX API hay userland runtime hoàn chỉnh.

## 2. Nguyên tắc thiết kế syscall set

*   **Small and deliberate**: Chỉ đưa vào syscall có giá trị kiến trúc rõ ràng. Không mở rộng theo thói quen “OS phải có”.
*   **Deterministic behavior**: Mọi syscall phải có hành vi nhất quán, dễ trace.
*   **No hidden authority transfer**: Syscall không được cho user khả năng thao túng privileged state (interrupt, mode bits, kernel memory).
*   **Clear failure modes**: Sai syscall number, sai args, sai pointer phải có outcome rõ ràng (error hoặc kill theo policy).

## 3. Syscall numbering

RefARM-OS dùng syscall number trong `r7` theo ABI. Numbering được chốt như sau:

| ID  | Name      | Description |
| --- | --------- | ----------- |
| 0   | sys_write | Output to UART |
| 1   | sys_exit  | Terminate task |
| 2   | sys_yield | Voluntary Reschedule (Optional) |

Các số khác là **invalid** trong scope hiện tại.

## 4. Syscall semantics

### 4.1. sys_write (id = 0)

**Purpose**
*   Cho phép user task output dữ liệu ra UART thông qua kernel.
*   Đây là syscall “proof of communication”: user không truy cập UART MMIO trực tiếp, mà phải đi qua kernel gate.

**Prototype (logical)**
```c
int sys_write(const void *buf, uint32_t len);
```

**ABI mapping**
*   `r7` = 0
*   `r0` = buf (user pointer, READ)
*   `r1` = len
*   `r0` (return) = bytes_written hoặc error (negative)

**Semantics**
*   Kernel đọc tối đa `len` bytes từ `buf` và ghi ra UART.
*   **Text Formatting**: Kernel thực hiện tự động chuyển đổi `\n` (LF) thành `\r\n` (CRLF) để đảm bảo hiển thị đúng trên terminal. User không cần tự thêm `\r`.
*   `sys_write` có thể ghi đủ hoặc ghi một phần (partial write) tùy thiết kế driver, nhưng phải deterministic.
*   Trong giai đoạn tối thiểu, khuyến nghị: hoặc ghi toàn bộ, hoặc fail (không cần streaming phức tạp).

**Memory rules**
`buf` là user pointer, phải tuân theo:
*   range validation
*   bounds validation
*   copy-in (khuyến nghị)
*   Kernel không dereference trực tiếp user pointer ngoài copy primitive.

**Bounds and limits**
*   `len` phải thỏa: `0 <= len <= MAX_WRITE_LEN`
*   `MAX_WRITE_LEN` là một hằng số kiến trúc (ví dụ 256 hoặc 512) và phải được công bố trong code + docs.
*   Nếu `len == 0`: return 0.

**Error conditions**
*   Invalid pointer/range: return `-4` (invalid user pointer) hoặc terminate task theo strict policy.
*   Invalid length (too large): return `-3` (invalid argument).
*   UART unavailable (nếu có tình huống này): return `-1` (generic failure) hoặc một code cụ thể nếu muốn.

**Side effects and invariants**
*   Không được thay đổi scheduler state.
*   Không được thay đổi interrupt policy ngoài việc cần thiết cho UART driver (khuyến nghị: không đụng IRQ state).
*   Không được block vô hạn.

### 4.2. sys_exit (id = 1)

**Purpose**
*   Cho phép user task kết thúc hợp lệ và trả quyền kiểm soát cho kernel/scheduler.
*   `exit` là bước hoàn thiện “task lifecycle” ở user mode: thay vì user cố tình fault để dừng, user có đường hợp lệ để chết.

**Prototype (logical)**
```c
void sys_exit(int32_t status);
```

**ABI mapping**
*   `r7` = 1
*   `r0` = status
*   **noreturn**

**Semantics**
*   Kernel đánh dấu current task là `DEAD`/`STOPPED`.
*   Kernel không return về user context của task đó nữa.
*   Kernel chuyển control sang scheduler để chọn task runnable khác (idle luôn runnable).

**Return behavior**
*   `sys_exit` là **noreturn**. User không bao giờ quay lại instruction sau SVC.

**Error conditions**
*   Trong scope tối thiểu, `exit` không fail. Nếu gọi được vào kernel thì kernel phải terminate task.
*   Nếu syscall entry bị gọi từ privileged mode (vi phạm contract), policy thuộc dispatch contract (panic hoặc error), nhưng không coi đây là “exit fail”.

**Side effects and invariants**
Must not corrupt scheduler invariants:
*   task removed khỏi runnable set theo đúng task model,
*   current task pointer được cập nhật nhất quán.
*   Không được để “dangling return” về task đã chết.

### 4.3. sys_yield (id = 2, optional)

**Purpose**
*   Cho phép user task chủ động nhường CPU để scheduler có điểm điều phối hợp lệ (voluntary scheduling point).
*   `yield` giúp hệ thống không phụ thuộc hoàn toàn vào timer IRQ để preempt (dù timer vẫn tồn tại).

**Prototype (logical)**
```c
int sys_yield(void);
```

**ABI mapping**
*   `r7` = 2
*   no args
*   return 0 hoặc error (trong scope tối thiểu: luôn 0)

**Semantics**
*   Kernel xem đây là yêu cầu “reschedule now”.
*   Kernel có thể:
    *   switch sang task khác, hoặc
    *   quyết định tiếp tục task hiện tại (nếu không có task runnable khác).
*   Dù quyết định thế nào, hành vi phải deterministic theo scheduler policy.

**Return behavior**
*   Nếu kernel quyết định không switch: return về user ngay sau SVC.
*   Nếu kernel switch: task có thể quay lại sau (khi được schedule lại), như một yield bình thường.

**Error conditions**
*   Trong scope tối thiểu, `yield` không fail.
*   Nếu syscall invalid context (privileged caller), policy thuộc dispatch contract.

**Side effects and invariants**
*   `yield` là syscall duy nhất được phép “kích hoạt scheduling” từ user.
*   Tuy nhiên, scheduling authority vẫn thuộc kernel; user chỉ yêu cầu, không quyết định.

## 5. Invalid syscalls

Nếu `r7` không thuộc `{0,1,2}` (hoặc `{0,1}` nếu chưa enable yield):
*   Kernel phải xử lý deterministic theo policy đã chốt trong dispatch contract, theo một trong hai hướng:
    *   Return error: `r0 = -2` (invalid syscall number)
    *   Strict: terminate task (coi như hostile behavior)
*   Dự án nên chọn một policy và ghi rõ trong implementation + docs.

## 6. Argument validation rules (summary)

*   **sys_write**:
    *   validate pointer range + bounds
    *   enforce `MAX_WRITE_LEN`
    *   copy-in trước khi output (khuyến nghị)
*   **sys_exit**:
    *   không cần validate status
*   **sys_yield**:
    *   không args

Mọi syscall đều phải tuân theo ABI: args ở `r0..r3`, syscall number ở `r7`, return ở `r0`.

## 7. Error codes (tối thiểu)

Dưới đây là bộ error codes tối thiểu (có thể mở rộng sau nhưng không được thay nghĩa):

| Code | Meaning |
| ---- | ------- |
| -1   | generic failure |
| -2   | invalid syscall number |
| -3   | invalid argument |
| -4   | invalid user pointer |

Nếu dự án chọn strong policy kill cho invalid syscall/pointer, error codes vẫn có thể được dùng cho logging (kernel trace) nhưng user có thể không nhận được return.

## 8. Observability requirements

Kernel phải trace tối thiểu:
*   syscall entry: task id/name, `r7`, `r0..r3`
*   syscall exit: `r0`, outcome (`OK`/`ERROR`/`KILL`)
*   riêng `exit`: trace action “task terminated” và next scheduled task (nếu có)

Trace là một phần của kiến trúc, không phải optional debug print.

## 9. Explicit non-goals

Tập syscall này không nhằm:
*   file I/O, VFS, filesystem syscalls
*   process management kiểu Unix
*   time syscalls, sleep, signals
*   networking syscalls
*   memory management syscalls (mmap/brk)
*   compatibility với Linux syscall table

## 10. Quan hệ với các tài liệu khác

*   **Dựa trên:**
    *   `01_syscall_scope.md`
    *   `02_svc_abi_contract.md`
    *   `03_syscall_dispatch_contract.md`
    *   `04_user_to_kernel_memory_rules.md`

*   **Đi kèm diagrams:**
    *   `syscall_dispatch`
    *   `svc_entry_flow`
    *   `user_pointer_validation`
    *   `syscall_return_path`