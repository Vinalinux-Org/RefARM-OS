# Scheduler Algorithm

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **scheduler algorithm** - logic quyết định task nào được chạy tiếp theo trong RefARM-OS.

Mục tiêu của scheduler algorithm:
- Định nghĩa chính sách scheduling (round-robin)
- Mô tả data structures quản lý tasks
- Giải thích scheduler decision flow
- Làm rõ khi nào và cách scheduler được gọi
- Cung cấp API scheduler cho kernel

Tài liệu này **không** mô tả:
- Task structure và lifecycle (xem `02_task_model.md`)
- Context switch implementation (xem `03_context_switch_contract.md`)
- Timer configuration (xem `05_timer_integration.md`)

---

## 2. Round-Robin Scheduling

### 2.1. Định nghĩa

**Round-robin** là thuật toán scheduling đơn giản nhất:
- Tasks được sắp xếp thành một queue circular
- Mỗi task chạy lần lượt theo thứ tự
- Khi task A kết thúc time slice → chuyển sang task B
- Sau task cuối → quay lại task đầu

### 2.2. Tại sao round-robin?

Lựa chọn round-robin vì:
- **Đơn giản:** Không cần priority, không cần complex decision
- **Fair:** Mọi task đều được CPU time đều đặn
- **Deterministic:** Dễ reasoning về behavior
- **No starvation:** Không có task nào bị bỏ quên

**Trade-offs:**
- Không có priority → không thể prioritize critical tasks
- Không có deadline → không phù hợp cho real-time
- Fixed time slice → không adaptive

### 2.3. Time Slice

**Time slice** (quantum) là khoảng thời gian mỗi task được chạy trước khi bị preempt.

**Trong RefARM-OS:**
- Time slice = timer tick period (khoảng 10ms)
- Mọi task có cùng time slice
- Không có dynamic adjustment

**Trade-offs của time slice:**
- **Quá ngắn** (<1ms): Context switch overhead cao
- **Quá dài** (>100ms): Response time kém
- **10ms:** Cân bằng hợp lý cho preemptive multitasking

---

## 3. Scheduler Data Structures

### 3.1. Task Array

Scheduler quản lý tasks thông qua một array tĩnh:

```c
#define MAX_TASKS 4

static struct task_struct tasks[MAX_TASKS];
static uint32_t task_count = 0;
```

**Properties:**
- Array có kích thước cố định
- Tasks được add tuần tự vào array
- Array index = task ID (implicit)

### 3.2. Current Task Pointer

Scheduler track task đang chạy:

```c
static struct task_struct *current_task = NULL;
```

**Invariants:**
- `current_task == NULL` → scheduler chưa start
- `current_task != NULL` → task đang chạy
- `current_task->state == RUNNING` (luôn đúng khi scheduler active)

### 3.3. Next Task Index

Scheduler sử dụng index để track task tiếp theo:

```c
static uint32_t next_task_index = 0;
```

**Round-robin logic:**
- `next_task_index` = index của task sẽ chạy sau current
- Khi switch: `next_task_index = (next_task_index + 1) % task_count`
- Circular: sau task cuối → về task đầu

---

## 4. Scheduler API

### 4.1. sched_init()

```c
void sched_init(void);
```

**Mục đích:**
- Khởi tạo scheduler data structures
- Reset task array và counters

**Preconditions:**
- Gọi trước khi tạo bất kỳ task nào

**Postconditions:**
- `task_count = 0`
- `current_task = NULL`

### 4.2. sched_add_task()

```c
int sched_add_task(void (*entry_point)(void),
                   void *stack_base,
                   uint32_t stack_size,
                   const char *name);
```

**Mục đích:**
- Tạo một task mới và add vào scheduler
- Khởi tạo task context

**Preconditions:**
- Scheduler đã init
- `task_count < MAX_TASKS`

**Postconditions:**
- Task được add vào `tasks[]` array
- Task state = READY
- `task_count` tăng 1

**Return:**
- `>= 0`: Task ID (array index)
- `< 0`: Error (task array full)

### 4.3. sched_start()

```c
void sched_start(void) __attribute__((noreturn));
```

**Mục đích:**
- Start scheduler
- Load first task và bắt đầu multitasking
- **Never returns**

**Preconditions:**
- Ít nhất 1 task đã được add
- Timer interrupt đã configured
- IRQ enabled

**Postconditions:**
- `current_task` = first task
- First task đang chạy

### 4.4. sched_tick()

```c
void sched_tick(void);
```

**Mục đích:**
- Gọi từ timer ISR mỗi khi tick xảy ra
- Thực hiện round-robin task switch

**Preconditions:**
- Scheduler đã start
- CPU ở interrupt context

**Postconditions:**
- Next task đang chạy
- `current_task` updated
- `next_task_index` advanced

---

## 5. Scheduler Decision Flow

### 5.1. sched_tick() logic

```c
void sched_tick(void) {
    // Guard: scheduler chưa start
    if (current_task == NULL) {
        return;
    }
    
    // Guard: chỉ có 1 task
    if (task_count <= 1) {
        return;
    }
    
    // Get next task
    struct task_struct *next = &tasks[next_task_index];
    struct task_struct *prev = current_task;
    
    // Update states
    prev->state = READY;
    next->state = RUNNING;
    
    // Update scheduler state
    current_task = next;
    next_task_index = (next_task_index + 1) % task_count;
    
    // Context switch (assembly)
    context_switch(prev, next);
    
    // ❌ NEVER REACHED - context_switch() never returns
}
```

### 5.2. CRITICAL: context_switch() Never Returns

**Execution flow kết thúc tại context_switch():**

```c
void sched_tick(void) {
    // ...
    context_switch(prev, next);
    
    // ❌ DEAD CODE - NEVER EXECUTED
    // uart_printf("After switch\n");  // BUG!
    // cleanup_resources();            // BUG!
    // return;                         // BUG!
}
```

**Tại sao never return?**
- context_switch() restore next task's PC
- CPU nhảy đến next task's code
- Stack bây giờ là next task's stack
- Không còn cách nào quay lại sched_tick()

**Common mistakes khi implement:**

```c
// ❌ WRONG: Gọi scheduler từ task code
void some_task(void) {
    while (1) {
        do_work();
        sched_tick();  // BUG! Never returns, task code sau bị bỏ qua
        more_work();   // Dead code
    }
}

// ❌ WRONG: Cleanup sau context_switch
void sched_tick(void) {
    context_switch(prev, next);
    free(temp_buffer);  // BUG! Never executed
}

// ✅ CORRECT: Chỉ gọi từ timer ISR, không có code sau
void timer_isr(void) {
    clear_interrupt();
    intc_eoi();
    sched_tick();  // OK - designed to not return
    // No code after this point
}
```

**Debug tip:**
Nếu bạn thấy code sau `context_switch()` hoặc `sched_tick()`, đó là bug.

### 5.3. Round-robin example

Với 3 tasks (A, B, C):

```
Tick 0: Current=A, Next=B
        A running...
        
Tick 1: Switch A→B
        B running...
        
Tick 2: Switch B→C
        C running...
        
Tick 3: Switch C→A (wrap)
        A running...
        
(repeat...)
```

**Properties:**
- Mỗi task chạy 1 lần trong cycle
- Cycle length = `task_count` ticks
- CPU time per task = `1 / task_count`

---

## 6. Edge Cases

### 6.1. Single task

```c
if (task_count <= 1) {
    return;  // No switch needed
}
```

**Behavior:**
- Không context switch
- Task chạy liên tục
- Timer vẫn fire (có thể dùng cho timekeeping)

### 6.2. Scheduler chưa start

```c
if (current_task == NULL) {
    return;  // Scheduler inactive
}
```

**Behavior:**
- Timer tick bị ignore
- Kernel chạy normal flow

### 6.3. Task array full

```c
if (task_count >= MAX_TASKS) {
    return -1;  // Error
}
```

**Behavior:**
- Task creation fail
- Caller phải handle error

---

## 7. Scheduler Invariants

Scheduler phải luôn duy trì:

### 7.1. At most one RUNNING task

```
Σ(task.state == RUNNING) <= 1
```

### 7.2. Current task consistency

```
current_task != NULL → current_task->state == RUNNING
```

### 7.3. Bounds checking

```
0 <= task_count <= MAX_TASKS
0 <= next_task_index < task_count
```

---

## 8. Performance Characteristics

### 8.1. Time complexity

**sched_tick():** O(1) - constant time
**sched_add_task():** O(1)
**sched_start():** O(1)

### 8.2. Space complexity

**Memory:** ~400 bytes total (4 tasks × 100 bytes/task)

### 8.3. Overhead

**Context switch:** ~100 cycles
**At 10ms time slice:** overhead < 0.001%

---

## 9. Initialization Sequence

Trong `kernel_main()`:

```c
sched_init();

sched_add_task(idle_task, idle_stack, STACK_SIZE, "idle");
sched_add_task(shell_task, shell_stack, STACK_SIZE, "shell");

sched_start();  // Never returns
```

**Order matters:**
- Idle task (index 0) chạy đầu tiên
- Round-robin: Idle → Shell → Idle → Shell

---

## 10. Debugging Helpers

### 10.1. Dump scheduler state

```c
void sched_dump_state(void) {
    uart_printf("Tasks: %u\n", task_count);
    uart_printf("Current: %s\n", current_task->name);
    
    for (uint32_t i = 0; i < task_count; i++) {
        uart_printf("  %s [%s]\n",
                    tasks[i].name,
                    tasks[i].state == RUNNING ? "RUN" : "RDY");
    }
}
```

### 10.2. Shell command

```c
static int cmd_ps(int argc, char **argv) {
    sched_dump_state();
    return 0;
}
```

---

## 11. Explicit Non-Goals

Scheduler algorithm **explicitly does NOT include**:

- Task priorities
- Priority inheritance
- Task sleep/wakeup
- Deadline scheduling
- Real-time guarantees
- CPU affinity
- Load balancing
- Dynamic task creation/destruction
- Task groups
- CPU time accounting

---

## 12. Quan hệ với các tài liệu khác

### 12.1. Phụ thuộc vào

- `02_task_model.md` - task structure
- `03_context_switch_contract.md` - context_switch()

### 12.2. Được trigger bởi

- `05_timer_integration.md` - timer ISR gọi sched_tick()

---

## 13. Tiêu chí thành công

Scheduler thành công khi:

### 13.1. Functional

- Tasks chạy xen kẽ theo round-robin
- Mọi task được CPU time fair
- Không có starvation

### 13.2. Code quality

- Code đơn giản, dễ hiểu
- Invariants được maintain
- Edge cases handled

### 13.3. Performance

- sched_tick() overhead < 1%
- Context switch < 100 cycles
- Deterministic behavior

---

## 14. Ghi chú thiết kế

- Scheduler phải **đơn giản nhất có thể**
- Round-robin đủ cho preemptive multitasking
- Không optimize prematurely
- Invariants phải được enforce
- Edge cases handle explicitly
- Debug hooks hữu ích cho development
- Extensions (priority, sleep) là future work
- Keep scheduler stateless

---