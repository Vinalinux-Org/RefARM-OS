# SVC ABI Contract

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa **ABI (Application Binary Interface)** cho system call của RefARM-OS khi user task gọi kernel thông qua SVC exception.

**Mục tiêu:**
*   Đóng băng “ngôn ngữ” giao tiếp user–kernel ở mức register-level.
*   Định nghĩa **syscall number**, **argument passing**, **return value**, và **error model**.
*   Định nghĩa quy tắc **preservation/clobber** để user code và kernel code có thể hợp tác mà không dựa vào ngầm định.
*   Đảm bảo syscall path traceable, deterministic, và tương thích với exception model của ARMv7-A.
*   Tài liệu này là **contract**. Implementation phải tuân theo.

## 2. Phạm vi và giả định

### 2.1. Phạm vi
*   ABI áp dụng cho mọi system call được thực hiện bằng `SVC` từ user execution.

### 2.2. Giả định
*   User task chạy ở **USR** (unprivileged).
*   Kernel mainline chạy ở **SVC** (privileged).
*   SVC entry là một nhánh của exception handling: `vector → entry stub → normalized context → dispatch`.

### 2.3. Ngoài phạm vi
*   ABI cho signal/IPC.
*   ABI cho preemption control hoặc thao tác privileged state.
*   POSIX/Linux compatibility đầy đủ.

## 3. Tổng quan ABI (một câu)

Một syscall trong RefARM-OS được mô tả như sau:
1.  User đặt **syscall number** vào `r7`.
2.  User đặt **arguments** vào `r0..r3` (tối đa 4 args trong scope hiện tại).
3.  User thực thi `SVC #0`.
4.  Kernel trả kết quả trong `r0` (và có thể dùng `r1` cho return phụ, nếu được syscall đó định nghĩa).
5.  Trường hợp lỗi: kernel trả giá trị âm trong `r0` theo error model.

## 4. Syscall Invocation Contract

### 4.1. Syscall instruction
*   User phải dùng: `SVC #0`.
*   Giá trị immediate của SVC trong scope này không mang ý nghĩa (luôn dùng 0 để thống nhất và tránh “kênh phụ”).
*   **Rationale**: syscall number đã nằm trong `r7`. SVC immediate không được dùng để encode syscall number.

### 4.2. Caller mode rule
*   Syscall hợp lệ chỉ khi SVC được gọi từ **USR mode**.
*   Nếu SVC được gọi từ privileged modes (SVC/IRQ/ABT/UND), đó là:
    *   bug trong kernel hoặc test harness,
    *   và phải có policy rõ ràng (panic hoặc return error). Policy này thuộc dispatch contract, nhưng ABI yêu cầu: không được coi là hợp lệ.

### 4.3. No implicit state channels
*   ABI không sử dụng **CPSR flags** như một kênh truyền ý nghĩa (ví dụ carry để báo lỗi).
*   ABI không dựa vào memory side effects để truyền kết quả trừ khi syscall semantics định nghĩa rõ.

## 5. Register Assignment

### 5.1. Syscall number
*   `r7` chứa syscall number (unsigned 32-bit).
*   Giá trị hợp lệ nằm trong tập syscall IDs mà kernel công bố trong `05_minimal_syscalls.md`.

### 5.2. Arguments
*   `r0..r3` là các argument slots chuẩn (tối đa 4 args).
*   **Quy tắc**:
    *   Argument là 32-bit value hoặc pointer.
    *   Nếu syscall cần 64-bit value: syscall đó phải định nghĩa packing rõ ràng (ví dụ dùng cặp `r0:r1` theo little-endian convention). Giai đoạn này khuyến nghị tránh 64-bit args.
*   **Reserve**:
    *   `r4..r6` được để dành cho tương lai (khi cần >4 args). Trong scope hiện tại, syscall không được yêu cầu caller truyền args qua `r4..r6`.

### 5.3. Return values
*   `r0` chứa return value chính.
*   `r1` chỉ được dùng nếu syscall đó định nghĩa return phụ (explicit). Nếu không, `r1` là undefined.

### 5.4. Syscall clobber rules (user-visible)

Sau khi syscall return về user, caller phải coi các register sau là **clobbered** (không được dựa vào giá trị cũ):
*   `r0..r3` (args slots, đồng thời chứa return)
*   `r7` (syscall number)
*   CPSR flags (N/Z/C/V) có thể thay đổi

Các register sau được coi là **preserved** theo contract tối thiểu (caller có thể kỳ vọng giữ nguyên):
*   `r4..r11` (callee-saved theo AAPCS, phù hợp với task model hiện tại)
*   `sp` (user stack pointer)
*   `lr` (return address trong user context)
*   `pc` (trừ khi syscall semantics định nghĩa không return, ví dụ `exit`)

**Lưu ý:**
*   Preservation là thuộc tính của end-to-end syscall path (entry stub + dispatch + handler + return). Kernel phải đảm bảo điều này bằng cách save/restore đúng context.

## 6. Error Model

### 6.1. Quy ước lỗi
*   Nếu syscall thành công: `r0 >= 0` (hoặc theo semantics cụ thể).
*   Nếu syscall lỗi: `r0` là số âm đại diện error code.
*   Ví dụ:
    *   `-1` : generic failure
    *   `-2` : invalid syscall number
    *   `-3` : invalid argument
    *   `-4` : invalid user pointer
    *   `-5` : permission denied (nếu cần)
*   Tập error codes chi tiết có thể được chốt sau, nhưng quy tắc “negative in r0” là bất biến của ABI.

### 6.2. Invalid syscall number
*   Nếu `r7` không thuộc tập syscall hợp lệ:
    *   Kernel phải xử lý deterministic:
        *   trả error (`r0 = -2`) hoặc
        *   terminate task theo policy strict (nếu system muốn coi là hostile)
*   ABI không bắt buộc policy cụ thể, nhưng bắt buộc: không được hành xử mơ hồ.

### 6.3. Syscall “does not return”
*   Một số syscall có thể được định nghĩa là **noreturn** (ví dụ `exit`):
    *   Nếu syscall semantics định nghĩa noreturn, kernel không được return về user.
    *   Caller phải coi mọi code sau syscall đó là unreachable.

## 7. Pointer and Memory Arguments (ABI-level rules)

ABI cho phép truyền pointer qua `r0..r3`, nhưng đặt ra nguyên tắc bắt buộc:
*   Kernel coi mọi pointer từ user là **không tin cậy**.
*   Kernel phải tuân thủ `04_user_to_kernel_memory_rules.md` để:
    *   validate/bounds-check,
    *   copy-in/copy-out,
    *   hoặc fail closed.
*   ABI không định nghĩa chi tiết validation, nhưng ABI khóa điều này như một invariant: pointer không được dereference “thẳng tay” trong kernel.

## 8. Observability Contract (syscall tracing fields)

Để syscall traceable, kernel syscall entry/dispatch phải có khả năng log tối thiểu các trường sau:
*   caller task id/name
*   syscall number (`r7`)
*   arguments `r0..r3` (có thể log dạng hex, và có thể redacted theo policy)
*   return value (`r0`)
*   outcome classification: `OK` / `ERROR` / `KILL` (nếu policy terminate)

Chi tiết vị trí log thuộc dispatch contract, nhưng ABI yêu cầu: syscall parameters phải có representation rõ ràng ở boundary.

## 9. Invariants (bất biến của ABI)

*   Syscall number nằm ở `r7`, arguments nằm ở `r0..r3`.
*   Syscall dùng `SVC #0`, immediate không encode syscall id.
*   Return value chính ở `r0`, lỗi được encode bằng số âm trong `r0`.
*   `r4..r11`, `sp`, `lr` của user context được preserve (trừ khi syscall noreturn).
*   Kernel không được dùng CPSR flags như kênh báo lỗi/semantics.

## 10. Explicit non-goals

ABI này không nhằm:
*   tương thích Linux syscall ABI đầy đủ (dù pattern gần giống).
*   hỗ trợ >4 args ngay lập tức.
*   định nghĩa `errno`, libc wrappers, hoặc calling convention cho userland runtime.

## 11. Quan hệ với các tài liệu khác

*   **Dựa trên:**
    *   exception model (SVC là một synchronous exception gate)
    *   privilege & cpu mode model
    *   mode transition contract (return path về user)
    *   system call scope

*   **Dẫn tới:**
    *   `03_syscall_dispatch_contract.md` (entry stub responsibilities, syscall table, dispatch invariants)
    *   `04_user_to_kernel_memory_rules.md` (pointer validation/copy rules)