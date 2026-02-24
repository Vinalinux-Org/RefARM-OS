# Privilege and CPU Mode Model

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa mô hình đặc quyền (privilege) và mô hình CPU mode (processor modes) khi dự án bắt đầu cho phép task chạy ở User mode trong khi kernel vẫn vận hành ở privileged mode.

**Mục tiêu cụ thể:**
*   Làm rõ RefARM-OS dùng những mode nào của ARMv7-A, và dùng để làm gì.
*   Định nghĩa ranh giới “kernel world” và “user world” ở mức CPU state.
*   Xác định các hệ quả kiến trúc của việc chuyển mode: banked registers, stack, SPSR/LR.
*   Cung cấp nền tảng để viết các contract ở tài liệu kế tiếp (mode transition, fault containment, syscall).

## 2. Phạm vi

**Phạm vi tài liệu chỉ bao gồm:**
*   **ARMv7-A privilege level** ở mức tối thiểu: PL0 vs PL1.
*   Các **processor modes** liên quan trực tiếp đến kernel/user và exception handling.
*   Cách RefARM-OS “đóng vai” các mode trong execution model.

**Ngoài phạm vi:**
*   Security Extensions (Secure/Non-secure, Monitor mode).
*   Virtualization Extensions (Hyp mode).
*   Debug exceptions và debug state.
*   Mô hình bảo mật/permission đầy đủ kiểu OS thương mại.

*(Ref: các mode Monitor/Hyp tồn tại trong ARMv7-A nhưng nằm ngoài mục tiêu dự án hiện tại - xem `ARMv7-A_System_Level_Training`)*

## 3. Khái niệm nền: Privilege level vs Processor mode

### 3.1. Privilege level (PL)
Trong RefARM-OS giai đoạn này, chỉ cần 2 mức:
*   **PL1 (Privileged):** Kernel world.
*   **PL0 (User):** User task world.

*Privilege level là góc nhìn “ai được phép làm gì” (ví dụ: thay đổi trạng thái CPU, thao tác tài nguyên kernel, truy cập memory mapping của kernel khi sau này bật MMU).*

### 3.2. Processor mode (CPU mode)
ARMv7-A có nhiều processor modes. Mode là trạng thái thực thi của core (USR, SVC, IRQ, ABT, UND, FIQ, SYS, …). Khi exception xảy ra, CPU tự động switch mode tương ứng và lưu thông tin return (LR) + trạng thái trước đó (SPSR).

*(Tham khảo: `03_exception_model` & `ARMv7-A_System_Level_Training`)*

## 4. Processor modes được dùng trong RefARM-OS

### 4.1. Kernel execution mode: SVC
Kernel “mainline execution” (không tính IRQ/abort handlers) chạy ở:
*   **SVC mode:** đây là privileged mode được ARM dùng làm “Supervisor” cho OS.

**Lý do chọn SVC:**
*   Đồng nhất với execution model hiện tại.
*   Khi có SVC exception (system call về sau), CPU cũng vào SVC mode theo kiến trúc, nên đường đi nhất quán.

### 4.2. User task mode: USR
User task chạy ở:
*   **USR mode:** unprivileged execution.
*   **CPSR Mode Bits (M[4:0]):** `10000` (`0x10`).

*USR mode không có SPSR riêng (SPSR tồn tại ở exception modes), nên việc quay lại user sau exception phải dựa vào SPSR của mode đang xử lý exception (SVC/IRQ/ABT/UND).*

### 4.3. Exception modes: IRQ/ABT/UND (FIQ không dùng)
Khi exception xảy ra, CPU có thể switch sang các mode:
*   **IRQ mode (`0x12`):** cho IRQ interrupt.
*   **ABT mode (`0x17`):** cho prefetch/data abort.
*   **UND mode (`0x1B`):** cho undefined instruction.
*   **FIQ mode (`0x11`):** tồn tại nhưng RefARM-OS hiện không dùng.

## 5. Register banking và ý nghĩa kiến trúc

### 5.1. Banked registers là gì?
ARMv7-A sử dụng register banking để việc vào exception nhanh và hạn chế phá vỡ context. 
Bảng tóm tắt các thanh ghi "ẩn" (banked) mà Kernel phải quản lý:

| Mode | R0-R12 | SP (R13) | LR (R14) | SPSR |
| :--- | :--- | :--- | :--- | :--- |
| **USR (0x10)** | Shared | **SP_usr** | **LR_usr** | *None* |
| **SVC (0x13)** | Shared | **SP_svc** | **LR_svc** | **SPSR_svc** |
| **IRQ (0x12)** | Shared | **SP_irq** | **LR_irq** | **SPSR_irq** |
| **ABT (0x17)** | Shared | **SP_abt** | **LR_abt** | **SPSR_abt** |
| **UND (0x1B)** | Shared | **SP_und** | **LR_und** | **SPSR_und** |

*Lưu ý: System Mode (SYS) dùng chung registers với USR nhưng là privileged. Ta hiện tại chưa dùng SYS.*

### 5.2. SPSR và LR theo mode
Khi exception xảy ra, CPU:
1.  Lưu CPSR hiện tại vào `SPSR_<mode>`.
2.  Lưu return address vào `LR_<mode>`.
3.  Switch sang exception mode tương ứng.

Điều này là nền tảng để RefARM-OS:
*   Có thể “đi vào kernel” từ user (do IRQ hoặc SVC về sau).
*   Có thể “đi ra user” bằng exception return dựa trên SPSR/LR đã được CPU ghi.

### 5.3. Stack tách theo exception mode
RefARM-OS đã xác định nguyên tắc:
*   Mỗi exception mode có dedicated stack (`IRQ_stack`, `ABT_stack`, `UND_stack`…).
*   SVC dùng kernel main stack.

Khi bắt đầu cho user mode, quy tắc này càng quan trọng vì:
*   Kernel không được “mượn” SP của user khi đang ở privileged exception context.
*   Exception entry phải là lớp chuẩn hoá context và stack discipline.

*(Tham khảo: `03_exception_model`)*

## 6. Model “Kernel world” và “User world” trong RefARM-OS

### 6.1. Kernel world
Kernel world gồm:
*   SVC mainline execution.
*   Exception handling contexts (IRQ/ABT/UND) khi CPU chuyển sang các mode đó.
*   Các thao tác thay đổi trạng thái hệ thống (interrupt mask, mode switch, vector base…) thuộc trách nhiệm privileged code path.

### 6.2. User world
User world gồm:
*   Task chạy ở **USR mode**.
*   Không được giả định khả năng điều khiển kernel execution flow bằng cách “động” tới kernel (giai đoạn này chưa có syscall, user task chỉ là “isolated execution”).

### 6.3. Ranh giới vào/ra
Ranh giới chuyển world xảy ra bởi CPU exception mechanism:
*   **User → Kernel:** qua IRQ (và sau này là SVC).
*   **Kernel → User:** qua exception return, khôi phục CPSR từ SPSR tương ứng.

*Điểm cốt lõi: kernel không “tự phát minh” cơ chế chuyển world; kernel bám sát hardware behavior của ARM exception model.*

## 7. Explicit assumptions

*   **Single-core:** không có concurrent kernel entry từ core khác.
*   **Kernel privileged execution** vẫn neo ở SVC mode như execution model đã định nghĩa.
*   **FIQ** không dùng trong giai đoạn này.
*   **Exception entry** là assembly layer chuẩn hoá context, không chứa “logic nghiệp vụ”.

## 8. Explicit non-goals

Tài liệu này không cố mô tả:
*   Một security model hoàn chỉnh (capabilities, access control list, sandboxing).
*   Virtual memory permission model (sẽ nằm ở tài liệu MMU mapping/permission về sau).
*   Syscall ABI (nằm ở phần system call).
*   Nested exceptions/re-entrancy nâng cao (chưa là mục tiêu hiện tại).

## 9. Liên kết tài liệu

*   **Execution model:** `04_kernel/01_execution_model.md`
*   **Exception model:** `04_kernel/03_exception_model.md` (CPU mode switching, SPSR/LR, stacks)
*   **Interrupt model:** `04_kernel/02_interrupt_model.md` (IRQ path và contract)