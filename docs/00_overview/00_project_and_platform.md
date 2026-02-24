# Hardware Assumptions

Tài liệu này mô tả các giả định về trạng thái phần cứng tại thời điểm kernel của dự án bắt đầu thực thi từ entry point `_start`.

Các giả định dưới đây là nền tảng để thiết kế và triển khai boot flow và kernel. Kernel **không tự thiết lập lại** các thành phần phần cứng được liệt kê, mà coi chúng là điều kiện tiên quyết đã được đảm bảo trước đó trong chuỗi boot.

---

## 1. CPU

1.1. SoC sử dụng CPU ARM Cortex-A8, kiến trúc ARMv7-A.

1.2. Hệ thống là single-core, không có SMP.

1.3. Khi kernel entry (`_start`) bắt đầu:
- CPU đang chạy ở chế độ privileged (SVC mode).
- IRQ và FIQ đã bị mask.
- Không có exception hoặc interrupt đang pending.

1.4. Kernel giả định:
- Không có core khác hoạt động song song.
- Không cần xử lý CPU bring-up hoặc core synchronization.

---

## 2. Memory

2.1. External DDR đã được khởi tạo và sẵn sàng sử dụng.

2.2. Địa chỉ base của DDR là `0x80000000`.

2.3. Kernel code, data, stack và toàn bộ runtime state của OS được đặt trong vùng DDR.

2.4. MMU đang ở trạng thái tắt (MMU OFF):
- Địa chỉ vật lý trùng với địa chỉ mà CPU truy cập.
- Không có paging hoặc address translation.

2.5. Cache (I-cache, D-cache) không được kernel giả định là đã bật.
Trạng thái cache không được sử dụng như một điều kiện tiên quyết cho correctness của kernel.

---

## 3. Clock và Reset

3.1. Clock tree của SoC đã được cấu hình trước khi kernel entry chạy.

3.2. Các clock cần thiết cho các peripheral cơ bản (ví dụ: UART dùng cho debug) đã hoạt động.

3.3. Kernel không thực hiện:
- PLL configuration
- Clock gating
- Peripheral clock enable trong giai đoạn boot ban đầu

3.4. Kernel giả định rằng hệ thống không ở trạng thái reset cục bộ tại thời điểm entry.

---

## 4. Peripheral cơ bản

4.1. UART0:
- Có thể được sử dụng ngay cho mục đích debug và printk.
- Clock và pinmux cho UART đã được cấu hình trước.

4.2. Watchdog:
- Có thể đang hoạt động.
- Kernel không giả định watchdog đã bị disable.
- Việc xử lý watchdog nằm ngoài phạm vi boot tối thiểu.

4.3. Các peripheral khác:
- Không được kernel sử dụng trong giai đoạn boot ban đầu.
- Không được giả định là đã được cấu hình.

---

## 5. Boot Environment

5.1. Kernel không phải là thành phần đầu tiên chạy sau reset.

5.2. Chuỗi boot trước kernel tồn tại và bao gồm:
- Boot ROM nội của SoC
- Bootloader (tạm thời sử dụng bản vendor)

5.3. Trách nhiệm của bootloader (ngoài phạm vi kernel):
- Khởi tạo DDR
- Cấu hình clock cơ bản
- Load kernel image vào RAM
- Chuyển quyền điều khiển sang kernel entry point

5.4. Kernel không phụ thuộc vào chi tiết triển khai cụ thể của bootloader, mà chỉ phụ thuộc vào trạng thái phần cứng tại thời điểm entry.

---

## 6. Phạm vi và giới hạn

6.1. Tài liệu này không mô tả:
- Thanh ghi phần cứng
- Chi tiết datasheet hoặc TRM
- Cách cấu hình bootloader

6.2. Mọi thông tin chi tiết về phần cứng nằm ngoài các giả định trên được tham chiếu tới tài liệu chính thức của SoC (Reference Manual).

6.3. Khi kernel phát triển thêm các giai đoạn boot hoặc tự đảm nhiệm thêm trách nhiệm phần cứng, các giả định trong tài liệu này phải được cập nhật tương ứng.

---
# RefARM-OS Strategic Roadmap

This document outlines the high-level evolution of the operating system.
Focus: **Architecture & Results** (Not implementation details).

## 🧱 PHASE 1: Trust & Stability (Scheduler)
> *"A kernel must be deterministic before it can be complex."*

*   **State**: The current scheduler works but is fragile and noisy.
*   **What we will do**:
    *   **Lockdown**: Define rigid rules for context switching (Contract).
    *   **Observe**: Add deep tracing to see the "heartbeat" of the system.
    *   **Enforce**: Panic immediately if rules are broken (Assertions).
*   **Result**: A **Trusted Kernel Core**. We stop hoping it works and start *knowing* it works.

## 🧱 PHASE 2: Isolation (User Mode Execution)
> *"The kernel must protect itself from its tasks."*

*   **State**: Currently, all tasks run as "Root" (SVC Mode). One bug crashes everything.
*   **What we will do**:
    *   **Drop Privilege**: Force specific tasks to run in User Mode (PL0).
    *   **Sever Links**: Ensure these tasks cannot access kernel memory or disable interrupts.
    *   **Silence**: These tasks will be unable to communicate (no syscalls yet).
*   **Result**: **Proof of Isolation**. We have a task running that physically cannot crash the kernel logic (though it shares memory map for now).

## 🧱 PHASE 3: Communication (System Calls)
> *"Isolation is useless without interaction."*

*   **State**: User tasks are isolated but mute/useless.
*   **What we will do**:
    *   **Build the Bridge**: Implement the SVC (Supervisor Call) entry gate.
    *   **Define the Language**: explicit rules for how User asks Kernel for help (ABI).
    *   **Enable I/O**: Allow User tasks to print to screen and exit.
*   **Result**: **A Working Operating System**. We have distinct Kernel and User worlds communicating safely.

## 🧱 PHASE 4: Independence (Bootloader)
> *"Own the hardware from the first instruction."*

*   **State**: We rely on external magic to load our kernel.
*   **What we will do**:
    *   **Take Control**: Write the raw assembly that runs immediately after the CPU wakes up.
    *   **Chain Load**: Manually load our kernel from storage to RAM.
*   **Result**: **Hardware Mastery**. Eliminate dependencies on U-Boot/third-party loaders.
