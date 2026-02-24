# Task Model

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **mô hình task (task model)** trong scheduler subsystem của RefARM-OS.

Mục tiêu của task model:
- Định nghĩa task là gì trong ngữ cảnh kernel
- Xác định cấu trúc dữ liệu lưu trữ trạng thái task
- Mô tả lifecycle và state transitions của task
- Làm rõ cách task được tạo và quản lý
- Cung cấp contract cho context switch mechanism

Tài liệu này **không** mô tả:
- Chi tiết triển khai context switch (xem `03_context_switch_contract.md`)
- Scheduler algorithm (xem `04_scheduler_algorithm.md`)
- Timer integration (xem `05_timer_integration.md`)

---

## 2. Định nghĩa task trong RefARM-OS

### 2.1. Task là gì?

Trong RefARM-OS:

**Task là:**
- Một luồng thực thi độc lập (execution context)
- Có register state riêng (r0-r12, sp, lr, SPSR)
- Có stack riêng
- Được scheduler điều phối để chạy xen kẽ với các task khác

**Task không phải:**
- Process (không có virtual memory, không có isolation)
- Thread theo nghĩa POSIX (không có user mode, không có privilege separation)
- Interrupt handler (mặc dù context switch xảy ra trong interrupt context)

### 2.2. Task vs Execution Flow

Task khác với "execution flow" đã được mô tả trong `04_kernel/01_execution_model.md`:

**Execution flow** mô tả cách kernel chạy như một tổng thể:
- Early kernel phase → Initialization phase → Steady execution phase
- Là khái niệm về lifecycle của kernel

**Task** là đơn vị thực thi trong steady execution phase:
- Kernel có thể chạy nhiều tasks
- Mỗi task có thể ở các state khác nhau
- Scheduler chuyển đổi giữa các tasks

---

## 3. Task State

Mỗi task có thể ở một trong các trạng thái sau:

### 3.1. READY

Task sẵn sàng chạy nhưng chưa được scheduler chọn.

**Đặc điểm:**
- Task có thể chạy ngay nếu được scheduler chọn
- Task không đang chờ bất kỳ sự kiện nào
- Register state đã được lưu, stack intact

### 3.2. RUNNING

Task đang được thực thi trên CPU.

**Đặc điểm:**
- Chỉ có **một** task ở trạng thái RUNNING tại mọi thời điểm (single-core)
- Task đang sử dụng CPU registers
- Task sẽ chạy cho đến khi timer tick trigger context switch

### 3.3. State Transition

```
       scheduler chọn task
READY ----------------------> RUNNING
  ^                              |
  |                              |
  |      timer tick xảy ra       |
  +------------------------------+
       context switch
```

**Quan trọng:**
- Task bị preempt bởi timer interrupt (involuntary, không tự nguyện yield)
- Chỉ có timer tick mới trigger context switch
- Không có BLOCKED state (không có synchronization primitives)

---

## 4. Task Structure

### 4.1. Saved CPU Context

Task structure phải lưu trữ toàn bộ CPU state để có thể restore khi task được chọn lại.

**Theo ARM Architecture Manual Part B (Exception Handling):**

Khi IRQ xảy ra (timer interrupt):
- Hardware tự động save `CPSR → SPSR_irq`  
- Hardware tự động save `PC → LR_irq` (với offset)
- Processor switch sang IRQ mode (banked r13_irq, r14_irq)

Context switch phải save/restore:
- **r0-r12**: General purpose registers (shared across modes)
- **SP (r13_svc)**: Task's stack pointer trong SVC mode
- **LR (r14_svc)**: Task's link register trong SVC mode  
- **SPSR**: Saved Program Status Register (task's CPSR frozen at interrupt point)

**Các registers cần lưu:**

```c
struct task_context {
    /* General purpose registers (13 registers) */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    
    /* Mode-specific registers (SVC mode) */
    uint32_t sp;      // r13_svc: Task's stack pointer
    uint32_t lr;      // r14_svc: Task's link register (return address)
    
    /* Processor state */
    uint32_t spsr;    // Saved Program Status Register (contains mode, flags, IRQ enable)
    
    // NOTE: PC is NOT saved separately
    // For interrupted tasks: PC is in LR_irq (handled by exception entry)
    // For new tasks: PC is in initial stack frame (loaded by RFE instruction)
};
```

**Giải thích SPSR vs CPSR:**

- **CPSR** (Current Program Status Register):  
  Current state của processor - mode bits, condition flags, IRQ enable, etc.
  
- **SPSR** (Saved Program Status Register):  
  Snapshot của CPSR **trước khi** exception xảy ra.  
  Mỗi exception mode có SPSR riêng: SPSR_irq, SPSR_fiq, SPSR_svc, etc.

Khi timer IRQ xảy ra:
```
1. Hardware: CPSR → SPSR_irq (save task's state)
2. Hardware: Set CPSR.M = IRQ mode (0b10010)
3. Hardware: Set CPSR.I = 1 (disable IRQ)
4. Context switch: Must save SPSR_irq to restore task's CPSR later
```

**Lưu ý về thứ tự fields:**
- Thứ tự này phải **match chính xác** với context switch assembly code
- Assembly code sẽ push/pop registers theo thứ tự này
- Nếu sai thứ tự → register values bị swap → crash

### 4.2. Task Control Block

Task control block chứa đầy đủ thông tin về task:

```c
struct task_struct {
    struct task_context context;    // Saved CPU state
    void *stack_base;               // Pointer to stack bottom
    uint32_t stack_size;            // Stack size in bytes
    uint32_t state;                 // READY or RUNNING
    const char *name;               // Task name (for debugging)
};
```

**Giải thích các fields:**

- **context:** Lưu registers khi task bị preempt (timer tick)
- **stack_base:** Pointer đến đáy stack (địa chỉ cao nhất, vì stack grows down)
- **stack_size:** Kích thước stack, dùng để detect overflow (future work)
- **state:** READY hoặc RUNNING
- **name:** Tên task, chỉ dùng cho debugging/logging

### 4.3. Memory Layout của Task

Mỗi task có vùng memory riêng cho stack:

```
High Address
     |
     v
+------------------+  <- stack_base
|                  |
|   Task Stack     |  (grows downward)
|                  |
+------------------+  <- sp (current stack pointer)
|                  |
|   Unused         |
|                  |
+------------------+  <- stack_base - stack_size
     |
     v
Low Address
```

**Stack properties:**
- Stack grows **downward** (ARM convention)
- Stack pointer (`sp`) giảm dần khi push data
- Stack overflow xảy ra khi `sp < (stack_base - stack_size)`

---

## 5. Task Lifecycle

### 5.1. Task Creation

Task được tạo qua hàm `task_create()`:

```c
int task_create(struct task_struct *task, 
                void (*entry_point)(void),
                void *stack_base,
                uint32_t stack_size,
                const char *name);
```

**Trách nhiệm của `task_create()`:**

1. Khởi tạo task structure
2. Setup initial context:
   - `pc` = entry_point (địa chỉ hàm task sẽ chạy)
   - `sp` = stack_base (đỉnh stack)
   - `lr` = 0 (task không bao giờ return)
   - `cpsr` = SVC mode với IRQ enabled
   - Các registers khác = 0
3. Set state = READY
4. Add task vào scheduler ready queue

**Sau khi tạo:**
- Task ở state READY
- Khi scheduler chọn task lần đầu, context switch sẽ load registers và nhảy đến entry_point

### 5.2. Task Entry Point

Entry point của task là một hàm với signature:

```c
void task_function(void) {
    // Task initialization
    
    // Task main loop (never returns)
    while (1) {
        // Do work
    }
}
```

**Contract:**
- Entry point **không bao giờ return**
- Nếu return → undefined behavior (lr = 0, sẽ crash)
- Task phải có vòng lặp vô hạn

### 5.3. Task Execution

Khi task đang chạy (RUNNING state):
- CPU registers chứa giá trị hiện tại của task
- Task structure không được access (context chưa saved)
- Task chạy cho đến khi timer tick xảy ra

### 5.4. Task Preemption (Timer Tick)

Khi timer interrupt xảy ra:

1. CPU nhảy vào timer ISR (IRQ mode)
2. Timer ISR clear interrupt flag
3. Timer ISR gọi `sched_tick()`
4. Scheduler:
   - Save current task context vào task structure
   - Set current task state = READY
   - Chọn next task theo round-robin
   - Load next task context từ task structure
   - Set next task state = RUNNING
5. Return từ interrupt → next task tiếp tục chạy

---

## 6. Static Task Allocation

### 6.1. Tại sao static allocation?

Trong RefARM-OS, tasks được allocate **tĩnh** tại compile time:

**Lý do:**
- Không có heap allocator (no malloc)
- Không có dynamic memory management
- Đơn giản hơn, dễ reasoning về memory usage
- Tránh fragmentation và allocation failures

### 6.2. Task Array

Kernel có một array cố định các tasks:

```c
#define MAX_TASKS 4

static struct task_struct tasks[MAX_TASKS];
static uint8_t task_count = 0;
```

**Giới hạn:**
- Số lượng tasks tối đa = `MAX_TASKS`
- Task creation fail nếu `task_count >= MAX_TASKS`
- Không thể destroy tasks sau khi tạo

### 6.3. Task Stack Allocation

Stack cho mỗi task cũng được allocate tĩnh:

```c
#define TASK_STACK_SIZE 4096  // 4KB per task

static uint8_t idle_task_stack[TASK_STACK_SIZE] __attribute__((aligned(8)));
static uint8_t shell_task_stack[TASK_STACK_SIZE] __attribute__((aligned(8)));
```

**Lưu ý:**
- Stack phải **aligned** (8-byte alignment cho ARM EABI)
- Stack size phải đủ lớn cho task's local variables và call stack
- Stack overflow detection **không có** trong giai đoạn hiện tại

---

## 7. Predefined Tasks

Kernel có 2 tasks được tạo tại boot time:

### 7.1. Idle Task

**Mục đích:**
- Chạy khi không có task nào khác có việc làm
- Tiết kiệm điện bằng cách gọi `wfi()` (Wait For Interrupt)

**Implementation:**

```c
void idle_task(void) {
    while (1) {
        wfi();  // Wait for interrupt, save power
    }
}
```

**Đặc điểm:**
- Luôn ở state READY (trừ khi đang RUNNING)
- Không bao giờ block
- Priority thấp nhất (nhưng không có priority mechanism, chỉ là conceptual)

### 7.2. Shell Task

**Mục đích:**
- Chạy interactive shell
- Poll UART RX buffer và xử lý commands

**Implementation:**

```c
void shell_task(void) {
    shell_init();
    
    while (1) {
        int c = uart_getc();
        if (c >= 0) {
            shell_process_char((char)c);
        }
        // No wfi() here - shell is always ready to process input
    }
}
```

**Đặc điểm:**
- Busy loop poll UART (không efficient, nhưng đơn giản)
- Không block khi không có input
- Chạy xen kẽ với idle task

---

## 8. Task Model Assumptions

### 8.1. Single-core execution

- Tại mọi thời điểm chỉ có **một** task RUNNING
- Không có parallel execution
- Context switch là atomic (xảy ra trong interrupt context với IRQ disabled)

### 8.2. No task priorities

- Tất cả tasks bình đẳng
- Round-robin scheduling
- Không có starvation (mỗi task được chạy đều đặn)

### 8.3. No task synchronization

- Không có mutex, semaphore, event
- Tasks không communicate với nhau
- Không có shared data protection mechanism

**Hệ quả:**
- Tasks phải tự quản lý shared resources (ví dụ: UART)
- Race conditions có thể xảy ra nếu tasks access cùng một resource

### 8.4. No blocking operations

- Tasks không có cơ chế block/wait
- Không có sleep() hoặc delay()
- Tasks luôn ở state READY hoặc RUNNING

---

## 9. Task Context Save/Restore Contract

### 9.1. Trách nhiệm của context switch

Context switch assembly code phải:

1. **Save current task:**
   - Push tất cả registers (r0-r12, sp, lr, pc, cpsr) lên stack
   - Copy stack frame vào `task->context`
   - Set task state = READY

2. **Load next task:**
   - Copy `task->context` vào registers
   - Set task state = RUNNING
   - Return từ interrupt (CPSR restored, jump to task's PC)

### 9.2. Stack frame layout

Khi save context, stack frame trông như sau:

```
High Address
     |
     v
+------------------+  <- sp trước khi save
|      CPSR        |
+------------------+
|       PC         |
+------------------+
|       LR         |
+------------------+
|       SP         |  (old SP value before interrupt)
+------------------+
|       R12        |
+------------------+
|       ...        |
+------------------+
|       R0         |
+------------------+  <- sp sau khi save (lowest address)
```

**Thứ tự này match với `struct task_context`.**

### 9.3. Critical constraints

**Memory barriers:**
- Phải có DSB (Data Synchronization Barrier) sau khi save context
- Đảm bảo memory writes hoàn tất trước khi switch task

**Atomicity:**
- Context switch phải atomic (không bị gián đoạn)
- IRQ đã bị disabled trong interrupt context
- Không cần thêm locking mechanism

---

## 10. Explicit Non-Goals

Task model này **explicitly does NOT include**:

- Dynamic task creation/destruction
- Task priorities
- Task sleep/wakeup
- Task blocking on events
- Inter-task communication
- Task synchronization (mutex, semaphore)
- Task affinity (SMP)
- Task CPU time accounting
- Stack overflow detection
- Task watchdog

Mọi nội dung nằm ngoài danh sách trên đều **không được giả định**.

---

## 11. Quan hệ với các tài liệu khác

### 11.1. Task model phụ thuộc vào

- `04_kernel/01_execution_model.md` - task là phần của steady execution phase
- `02_boot/boot_contract.md` - task stack allocation tuân theo memory layout

### 11.2. Task model là nền tảng cho

- `03_context_switch_contract.md` - context switch làm việc với task structure
- `04_scheduler_algorithm.md` - scheduler quản lý task state transitions
- `05_timer_integration.md` - timer trigger task preemption

### 11.3. Task model không thay thế

- Execution model (task là refinement, không phải replacement)
- Interrupt model (task preemption vẫn tuân theo interrupt model)

---

## 12. Tiêu chí thành công của task model

Task model được coi là thành công khi:

### 12.1. Functional requirements

- Có thể tạo nhiều tasks với stack riêng
- Task structure chứa đủ thông tin để save/restore context
- Task lifecycle rõ ràng (creation → READY → RUNNING → READY → ...)

### 12.2. Code quality requirements

- Task structure layout đơn giản, dễ hiểu
- Task creation API rõ ràng, khó dùng sai
- Task stack alignment đúng ARM EABI requirements

### 12.3. Documentation requirements

- Task structure được document đầy đủ
- Stack layout được minh họa rõ ràng
- Contract với context switch được nêu explicit

---

## 13. Ghi chú thiết kế

- Task model phải **đơn giản và deterministic**
- Không thêm fields vào task structure trừ khi có nhu cầu rõ ràng
- Stack size phải đủ lớn cho worst-case usage
- Task entry point **không bao giờ return** - đây là hard contract
- Task structure layout **must match** context switch assembly code
- Thay đổi task structure → phải update context switch code đồng bộ

---