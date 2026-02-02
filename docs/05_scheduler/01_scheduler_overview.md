# Scheduler Overview

## 1. Mục tiêu

Tài liệu này mô tả scheduler subsystem của dự án RefARM-OS - thêm **preemptive multitasking với round-robin scheduling** vào kernel.

Mục tiêu cụ thể:
- Cho phép nhiều task chạy xen kẽ trên single-core CPU
- Sử dụng timer interrupt làm điểm chuyển đổi giữa các task (preemption)
- Triển khai round-robin scheduler đơn giản
- Giữ được tính đơn giản và deterministic của kernel

Scheduler subsystem **không** triển khai:
- Task priorities (chỉ có round-robin)
- Dynamic task creation
- Inter-task communication
- Synchronization primitives (mutex, semaphore)

---

## Lưu ý về thuật ngữ

**Trong tài liệu này, scheduler được gọi là "preemptive" vì:**
- Tasks bị gián đoạn bởi timer interrupt (involuntary preemption)
- Tasks **không** tự nguyện yield CPU

**Trong OS literature chuẩn:**
- **Cooperative multitasking** = tasks tự gọi yield() để nhường CPU
- **Preemptive multitasking** = kernel force context switch (như RefARM-OS)

RefARM-OS là **preemptive**, không phải cooperative.

---

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi của scheduler subsystem

Scheduler subsystem được thêm vào sau khi các thành phần sau đã hoạt động:
- IRQ infrastructure đã hoạt động ổn định
- UART RX interrupt đã được test
- Exception handling đã được triển khai
- Kernel có thể enable IRQ và xử lý interrupt an toàn

### 2.2. Các thành phần đã có trước đó

Các thành phần sau đã được triển khai trước khi thêm scheduler:
- Boot sequence (reset → entry → kernel_main)
- Exception infrastructure (vector table, exception handlers)
- Interrupt infrastructure (INTC driver, IRQ dispatch, EOI protocol)
- UART driver với RX interrupt
- Shell với command dispatcher

### 2.3. Dependencies

Scheduler subsystem phụ thuộc vào các tài liệu và thành phần sau:
- `02_boot/boot_contract.md` - boot contract không thay đổi
- `04_kernel/01_execution_model.md` - scheduler là phần mở rộng của execution model
- `04_kernel/02_interrupt_model.md` - timer interrupt tuân theo interrupt model
- `04_kernel/05_interrupt_infrastructure.md` - timer ISR sử dụng IRQ framework

---

## 3. Preemptive Multitasking Model

### 3.1. Preemptive Scheduling

Trong dự án này, scheduler sử dụng **preemptive multitasking**:

**Preemptive nghĩa là:**
- Task bị gián đoạn bởi timer interrupt (involuntary)
- Task không tự quyết định khi nào nhường CPU
- Context switch xảy ra tại điểm cố định (mỗi timer tick)

**Khác với cooperative:**
- Cooperative: task tự gọi `yield()` để nhường CPU
- Preemptive: kernel force switch qua timer interrupt

### 3.2. Round-Robin Policy

Scheduler sử dụng round-robin:
- Không có task priorities
- Tasks chạy lần lượt theo thứ tự vòng tròn
- Fair: mỗi task nhận CPU time đều đặn

### 3.3. Timeline Visualization

**Execution timeline với 2 tasks:**

```text 
Timeline  : t0              t1                 t2                 t3
          : |               |                  |                  |
-------------------------------------------------------------------
TASK A    : [==== RUN ====] :                  : [==== RUN ====]  :          
          :                 :                  :        ^         :
          :                 :   [ Systick ]    :        |         :
          :                 :        |         :        |         :
          :                 :        v         :        |         :
          :                 :  [ Save Ctx A ]  :  [ Save Ctx B ]  :          
          :                 :  [ Scheduler  ]  :  [ Scheduler  ]  :           
          :                 :  [ Rest Ctx B ]  :  [ Rest Ctx A ]  :       
          :                 :        |         :        ^         : 
          :                 :        |         :        |         :
          :                 :        v         |   [ Systick ]    :         
TASK B    : . . . . . . . . : [==== RUN ====]  :                  :             
          :                 :                  :                  : 
```

**Điều gì xảy ra mỗi timer tick:**

```
Tick 0 (10ms):
  - Task A đang chạy
  - Timer interrupt xảy ra
  - Save context A (r0-r12, sp, lr, cpsr)
  - Load context B
  - Task B bắt đầu chạy
  
Tick 1 (20ms):
  - Task B đang chạy
  - Timer interrupt xảy ra
  - Save context B
  - Load context A
  - Task A tiếp tục (từ chỗ bị preempt)
  
Tick 2 (30ms):
  - Cycle lặp lại...
```

**Key insight:**
- Mỗi task "tưởng" nó chạy liên tục
- Thực tế: task bị pause/resume liên tục
- Context save/restore làm cho việc pause/resume transparent

### 3.4. Tại sao preemptive round-robin?

Lựa chọn này vì:
- Đơn giản hơn nhiều so với priority-based preemptive
- Đủ cho mục tiêu của dự án (reference OS, không phải production RTOS)
- Dễ debug và dễ reasoning về kernel behavior
- Fair scheduling - không có task nào bị starve

---

## 4. Ba thành phần chính của scheduler subsystem

Scheduler subsystem bao gồm 3 thành phần chính, được triển khai theo 3 giai đoạn:

### 4.1. Task Representation

**Mục tiêu:**
- Định nghĩa task là gì trong kernel
- Thiết kế cấu trúc dữ liệu lưu trạng thái CPU của task
- Triển khai task creation mechanism

**Chi tiết trong:** `02_task_model.md`

**Key concepts:**
- Task structure chứa saved registers (r0-r12, sp, lr, SPSR)
- Mỗi task có static stack riêng
- Task state: READY, RUNNING
- Task không phải process (no virtual memory, no protection)

### 4.2. Context Switch Mechanism

**Mục tiêu:**
- Triển khai assembly code để save/restore CPU state
- Đảm bảo task này tạm dừng, task kia tiếp tục chạy một cách seamless

**Chi tiết trong:** `03_context_switch_contract.md`

**Key concepts:**
- Save current task registers vào task structure
- Load next task registers từ task structure
- ARM mode switching (IRQ mode → SVC mode)
- Stack frame layout phải match chính xác với task structure

### 4.3. Scheduler Algorithm

**Mục tiêu:**
- Quyết định task nào chạy tiếp theo
- Triển khai round-robin policy

**Chi tiết trong:** `04_scheduler_algorithm.md`

**Key concepts:**
- Round-robin: các task chạy lần lượt, không có priority
- Scheduler được gọi từ timer ISR
- Scheduler gọi context switch để chuyển task

---

## 5. Timer Integration - Trigger Mechanism

Scheduler cần một cơ chế để trigger việc chuyển đổi task. Cơ chế này là **timer interrupt**.

**Mục tiêu:**
- Sử dụng DMTimer2 để tạo periodic interrupt
- Timer ISR gọi scheduler mỗi khi tick xảy ra
- Fix bug thiếu clock enable trong timer driver hiện tại

**Chi tiết trong:** `05_timer_integration.md`

**Flow tổng thể:**
```
DMTimer2 overflow
    ↓
IRQ exception
    ↓
exception_entry_irq (assembly)
    ↓
irq_dispatch() (kernel IRQ core)
    ↓
timer_isr() (driver)
    ↓
sched_tick() (scheduler)
    ↓
context_switch() (assembly)
    ↓
Return to next task
```

---

## 6. Ba giai đoạn triển khai

Scheduler subsystem được triển khai theo 3 giai đoạn tuần tự:

### Giai đoạn 1: Task Representation và Context Switch Assembly

**Files cần tạo:**
- `include/task.h` - task structure definition
- `arch/arm/context_switch.S` - assembly save/restore stubs
- `kernel/task.c` - task creation helpers

**Mục tiêu:**
- Có cấu trúc dữ liệu task
- Có assembly code context switch
- Test context switch bằng cách manually gọi (chưa có scheduler)

**Tiêu chí hoàn thành:**
- Có thể tạo 2 tasks
- Có thể manually switch giữa chúng
- Tasks chạy đúng code và không crash

### Giai đoạn 2: Scheduler Logic và Timer Integration

**Files cần tạo/sửa:**
- `include/scheduler.h` - scheduler API
- `kernel/scheduler.c` - round-robin implementation
- `drivers/timer.c` - fix clock enable bug, hook vào scheduler

**Mục tiêu:**
- Scheduler quyết định task nào chạy tiếp
- Timer ISR trigger scheduler mỗi tick
- Idle task chạy khi không có task nào khác

**Tiêu chí hoàn thành:**
- Timer tick trigger scheduler
- Scheduler chọn task tiếp theo đúng round-robin
- Context switch xảy ra tự động mỗi tick

### Giai đoạn 3: Integration và Boot Sequence

**Files cần sửa:**
- `kernel/main.c` - tạo tasks, start scheduler

**Mục tiêu:**
- Tích hợp scheduler vào kernel boot sequence
- Shell chạy như một task
- Idle task chạy khi shell đang chờ input

**Tiêu chí hoàn thành:**
- Kernel boot → tạo tasks → start scheduler → tasks chạy xen kẽ
- Shell vẫn hoạt động bình thường
- Có thể add shell command để xem task status

---

## 7. Execution Flow sau khi có scheduler

Sau khi scheduler hoàn thành, kernel execution flow sẽ như sau:

### 7.1. Boot sequence (không đổi)

```
Reset → reset.S → entry.S → kernel_main()
```

Boot contract không thay đổi.

### 7.2. Kernel initialization (có thêm scheduler init)

```
kernel_main() {
    watchdog_disable();
    uart_init();
    intc_init();
    irq_init();
    uart_enable_rx_interrupt();
    timer_init();           // Đã có
    
    // NEW: Scheduler additions
    sched_init();
    task_create(idle_task);
    task_create(shell_task);
    sched_start();          // Never returns
}
```

### 7.3. Steady execution (multitasking)

```
Timer tick (mỗi ~10ms)
    ↓
Timer ISR
    ↓
sched_tick()
    ↓
context_switch()
    ↓
Next task continues execution
```

**Các tasks:**
- **Idle task:** vòng lặp `wfi()` để tiết kiệm điện
- **Shell task:** `uart_getc()` → `shell_process_char()` → xử lý commands

---

## 8. Giả định kiến trúc

Scheduler subsystem dựa trên các giả định sau:

### 8.1. Single-core preemptive multitasking

- Kernel vẫn chạy trên single-core
- Tại mọi thời điểm chỉ có một task đang chạy
- Context switch chỉ xảy ra tại timer tick (involuntary preemption)

### 8.2. No memory protection

- Tất cả tasks chạy ở SVC mode
- Không có user mode
- Tasks có thể truy cập bất kỳ memory nào
- Không có MMU protection

### 8.3. Static task allocation

- Số lượng tasks cố định tại compile time
- Không có dynamic task creation/destruction
- Task stacks được allocate tĩnh trong `.bss` hoặc dedicated section

### 8.4. No task priorities

- Tất cả tasks đều bình đẳng
- Round-robin scheduling
- Không có starvation prevention cần thiết (vì round-robin fair)

---

## 9. Explicit Non-Goals

Scheduler subsystem **explicitly does NOT include**:

- Preemptive multitasking
- Task priorities hoặc priority inheritance
- Dynamic task creation/destruction
- Task synchronization primitives (mutex, semaphore, event)
- Inter-task communication (IPC, message queues)
- Task sleep/wakeup mechanism
- Deadline scheduling hoặc real-time guarantees
- SMP hoặc multi-core scheduling
- Virtual memory hoặc process isolation
- Stack overflow detection

Mọi nội dung nằm ngoài danh sách trên đều **không được giả định** khi đọc tài liệu này.

---

## 10. Quan hệ với các tài liệu khác

### 10.1. Scheduler subsystem dựa trên

- `02_boot/boot_contract.md` - boot assumptions không đổi
- `04_kernel/01_execution_model.md` - scheduler mở rộng execution model
- `04_kernel/02_interrupt_model.md` - timer interrupt tuân theo interrupt model
- `04_kernel/05_interrupt_infrastructure.md` - sử dụng IRQ framework

### 10.2. Scheduler subsystem bao gồm

- `05_scheduler/02_task_model.md` - task abstraction
- `05_scheduler/03_context_switch_contract.md` - context switch ABI
- `05_scheduler/04_scheduler_algorithm.md` - round-robin logic
- `05_scheduler/05_timer_integration.md` - timer ISR integration

### 10.3. Scheduler subsystem không thay thế

- Boot flow và boot contract
- Exception handling model
- Interrupt infrastructure
- Driver model

Mọi thay đổi làm ảnh hưởng đến execution model hoặc interrupt model phải được đồng bộ với các tài liệu liên quan.

---

## 11. Tiêu chí thành công

Scheduler subsystem được coi là thành công khi:

### 11.1. Functional requirements

- Kernel có thể chạy nhiều tasks xen kẽ
- Timer interrupt trigger context switch đều đặn
- Shell task vẫn hoạt động bình thường
- Idle task chạy khi không có task nào khác ready
- Kernel không crash sau khi chạy lâu dài

### 11.2. Code quality requirements

- Context switch code phải đơn giản, dễ đọc
- Scheduler algorithm rõ ràng, không có magic numbers
- Task structure layout được document đầy đủ
- Không có hardcoded assumptions về số lượng tasks

### 11.3. Documentation requirements

- Tất cả 5 files trong `05_scheduler/` được viết đầy đủ
- Diagrams phản ánh đúng implementation
- Code comments giải thích các critical sections
- Testing strategy cho từng giai đoạn được document

---

## 12. Lộ trình đọc tài liệu scheduler

Để hiểu đầy đủ scheduler subsystem, đọc theo thứ tự sau:

1. `01_scheduler_overview.md` (file này) - hiểu big picture
2. `02_task_model.md` - hiểu task structure và lifecycle
3. `03_context_switch_contract.md` - hiểu cơ chế save/restore
4. `04_scheduler_algorithm.md` - hiểu round-robin logic
5. `05_timer_integration.md` - hiểu timer trigger mechanism

Sau khi đọc xong 5 files, nên xem các diagrams trong `diagram/` để có visual representation.

---

## 13. Ghi chú thiết kế

- Scheduler phải **đơn giản, rõ ràng và deterministic**
- Không mở rộng scheduler để "đón đầu" các tính năng chưa có
- Mọi mở rộng phải xuất phát từ **nhu cầu kiến trúc rõ ràng**
- Scheduler **không được** trở thành workaround cho kernel design issues
- Preemptive round-robin đủ cho mục tiêu reference OS
- Nếu cần priority scheduling sau này, đó là subsystem mới, không phải mở rộng hiện tại

---