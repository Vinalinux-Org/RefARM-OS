# Context Switch Contract

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **context switch contract** - giao ước giữa scheduler và assembly code thực hiện chuyển đổi task.

Mục tiêu của context switch contract:
- Định nghĩa rõ ràng cách CPU state được save và restore
- Xác định register usage convention
- Mô tả stack frame layout phải tuân theo
- Làm rõ memory barrier requirements
- Cung cấp specification đầy đủ để implement context switch assembly

Tài liệu này **không** mô tả:
- Scheduler algorithm (xem `04_scheduler_algorithm.md`)
- Task lifecycle management (xem `02_task_model.md`)
- Timer integration (xem `05_timer_integration.md`)

---

## 2. Context Switch là gì?

### 2.1. Định nghĩa

**Context switch** là quá trình:
- Tạm dừng task hiện tại (save CPU state)
- Chuyển sang task khác (restore CPU state)
- Làm cho task khác tiếp tục thực thi như thể không bị gián đoạn

### 2.2. Khi nào context switch xảy ra?

Context switch xảy ra trong timer interrupt handler:

```
Timer tick
    ↓
Timer ISR (IRQ mode)
    ↓
sched_tick() (C code)
    ↓
context_switch() (assembly)
    ↓
Return to next task (SVC mode)
```

**Quan trọng:**
- Context switch **chỉ** xảy ra trong timer ISR
- CPU đang ở IRQ mode khi gọi context_switch()
- Sau context switch, CPU trở về SVC mode (task mode)

### 2.3. CRITICAL: CPU Mode vs Execution Context

**Phân biệt rõ ràng:**

```
┌─────────────────────────────────────────────┐
│ Timer ISR (IRQ mode, IRQ stack)             │
│   - Clear interrupt                         │
│   - Call sched_tick()                       │
│   - Call context_switch()                   │
│     └─> Switch CPU to SVC mode ◄─── ĐÂY    │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│ Task continues (SVC mode, TASK STACK)       │
│   - Registers restored từ task context     │
│   - Stack = task's own stack               │
│   - NOT interrupt stack                    │
└─────────────────────────────────────────────┘
```

**Key points người mới hay nhầm:**
- Context switch CODE chạy trong IRQ context (IRQ mode, IRQ stack)
- Nhưng task SAU SWITCH chạy ở SVC mode với task's own stack
- IRQ stack chỉ dùng trong timer_isr + sched_tick + context_switch entry
- Task KHÔNG "chạy trong interrupt" - task chạy ở SVC mode bình thường

### 2.4. Tại sao cần assembly?

Context switch **phải** viết bằng assembly vì:
- Cần manipulate tất cả CPU registers trực tiếp
- Cần switch CPU mode (IRQ → SVC)
- Cần restore CPSR và PC atomically
- C compiler không thể generate code cho các operations này

---

## 3. Context Switch Entry Points

Context switch assembly cung cấp 2 entry points:

### 3.1. context_switch()

```assembly
void context_switch(struct task_struct *current, 
                    struct task_struct *next);
```

**Sử dụng:**
- Gọi từ scheduler khi switch giữa 2 tasks đã running
- Save context của `current` task
- Load context của `next` task

**Preconditions:**
- `current` đang ở state RUNNING
- `next` đang ở state READY
- CPU ở IRQ mode
- IRQ đã bị disabled

**Postconditions:**
- `current` context đã được saved vào `current->context`
- `next` context đã được loaded từ `next->context`
- CPU ở SVC mode, đang chạy `next` task
- IRQ enabled (CPSR restored từ next->context.cpsr)

### 3.2. start_first_task()

```assembly
void start_first_task(struct task_struct *first);
```

**Sử dụng:**
- Gọi từ `sched_start()` lần đầu tiên
- **Chỉ load** context, không save (vì chưa có current task)
- Never returns

**Preconditions:**
- `first` task đã được tạo và ở state READY
- CPU ở SVC mode
- IRQ disabled

**Postconditions:**
- `first` context đã được loaded
- CPU đang chạy `first` task
- IRQ enabled (theo first->context.cpsr)

---

## 4. CPU State Requirements

### 4.1. Registers phải save/restore

Context switch phải save/restore **tất cả** các registers sau:

```
r0  - r12   : General purpose registers (13 registers)
sp          : Stack pointer (r13)
lr          : Link register (r14)
pc          : Program counter (r15)
cpsr        : Current Program Status Register
```

**Tổng cộng: 17 values (68 bytes)**

### 4.2. Registers không cần save

Các registers sau **không** cần save vì chúng là banked registers của CPU modes:

```
spsr        : Saved Program Status Register (banked per mode)
sp_irq      : IRQ mode stack pointer (kernel không switch IRQ stack)
lr_irq      : IRQ mode link register (kernel quản lý riêng)
```

**Lý do:**
- Task chạy ở SVC mode, không có banked registers riêng
- IRQ mode registers là của kernel, không phải của task

### 4.3. CPSR fields quan trọng

Khi save/restore CPSR, các fields sau có ý nghĩa:

```
Bits [4:0]  : Mode bits (phải là 0x13 = SVC mode)
Bit  [5]    : Thumb bit (phải là 0 = ARM mode)
Bit  [6]    : FIQ disable (phải là 1 = FIQ masked)
Bit  [7]    : IRQ disable (phải là 0 = IRQ enabled sau khi switch)
Bits [31:28]: Condition flags (N, Z, C, V)
```

**Critical:**
- Task luôn chạy ở **SVC mode, ARM state**
- IRQ **must be enabled** trong task context (bit 7 = 0)
- FIQ **must be disabled** (bit 6 = 1)

---

## 5. Stack Frame Layout

### 5.1. Save sequence

Khi save context, assembly code push registers lên stack theo thứ tự:

```
push {r0-r12, lr}    ; Save general registers
mrs  r0, cpsr        ; Get CPSR
push {r0}            ; Save CPSR
```

**Stack layout sau khi save:**

```
High Address
     |
     v
+------------------+  <- sp before save
|      r0          |
+------------------+
|      r1          |
+------------------+
|      r2          |
+------------------+
|      ...         |
+------------------+
|      r12         |
+------------------+
|      lr          |
+------------------+
|      cpsr        |
+------------------+  <- sp after save (lowest address)
```

### 5.2. Mapping với task_context structure

Stack frame phải **match chính xác** với `struct task_context`:

```c
struct task_context {
    uint32_t cpsr;    // <- sp[0] (lowest address)
    uint32_t lr;      // <- sp[1]
    uint32_t r12;     // <- sp[2]
    uint32_t r11;     // <- sp[3]
    // ...
    uint32_t r1;      // <- sp[13]
    uint32_t r0;      // <- sp[14] (highest address in frame)
};
```

**Critical:**
- Thứ tự fields trong struct **phải** match thứ tự push/pop
- Nếu sai thứ tự → registers bị swap → crash
- Assembly code và C struct phải được maintain đồng bộ

### 5.3. PC và SP handling

**PC (Program Counter):**
- Không cần explicitly save vì `lr` đã chứa return address
- Khi restore, `lr` được load vào PC để jump đến task

**SP (Stack Pointer):**
- SP hiện tại của task được save bằng cách copy `sp` register
- Khi restore, `sp` được set lại từ saved value

---

## 6. Context Switch Implementation Pseudocode

### 6.1. context_switch() pseudocode

```assembly
context_switch:
    ; ===== SAVE CURRENT TASK =====
    
    ; 1. Switch to SVC mode (task mode) temporarily
    ;    để access task's stack
    cps  #0x13              ; Switch to SVC mode
    
    ; 2. Save all registers on current task's stack
    push {r0-r12, lr}
    
    ; 3. Save CPSR
    mrs  r0, cpsr
    push {r0}
    
    ; 4. Save SP into current->context.sp
    ;    (current pointer vẫn còn trong register từ C call)
    mov  r1, sp
    str  r1, [r0, #OFFSET_CONTEXT_SP]
    
    ; 5. Copy stack frame vào current->context
    ;    (implementation-specific)
    
    ; ===== LOAD NEXT TASK =====
    
    ; 6. Load next task's SP
    ldr  sp, [r1, #OFFSET_CONTEXT_SP]
    
    ; 7. Restore CPSR
    pop  {r0}
    msr  cpsr, r0
    
    ; 8. Restore all registers
    pop  {r0-r12, pc}       ; PC restored từ saved LR
```

### 6.2. start_first_task() pseudocode

```assembly
start_first_task:
    ; ===== LOAD FIRST TASK (NO SAVE) =====
    
    ; 1. Load task's SP
    ldr  sp, [r0, #OFFSET_CONTEXT_SP]
    
    ; 2. Restore CPSR
    pop  {r1}
    msr  cpsr, r1
    
    ; 3. Restore all registers và jump
    pop  {r0-r12, pc}       ; Never returns
```

---

## 7. ARM Mode Switching

### 7.1. Tại sao cần switch mode?

Context switch được gọi từ timer ISR, CPU đang ở **IRQ mode**. Nhưng tasks chạy ở **SVC mode**.

**Do đó phải:**
- Switch từ IRQ mode → SVC mode để access task's stack
- Restore task's CPSR để trở về SVC mode khi return

### 7.2. CPS instruction

ARM cung cấp `CPS` (Change Processor State) instruction:

```assembly
cps  #0x13    ; Change to SVC mode (mode bits = 0x13)
```

**Effects:**
- CPSR mode bits [4:0] = 0x13
- CPU switch sang SVC mode
- SP và LR bây giờ là SVC mode's banked registers

### 7.3. CPSR restore

Khi restore CPSR từ saved context:

```assembly
msr  cpsr_cxsf, r0    ; Restore all CPSR fields
```

**CPSR fields:**
- `c` (control): Mode bits, IRQ/FIQ masks
- `x` (extension): Reserved
- `s` (status): Reserved  
- `f` (flags): Condition flags (N, Z, C, V)

**Effect:**
- CPU mode restored (về SVC)
- IRQ enabled bit restored (IRQ enabled trong task)
- Condition flags restored

---

## 8. Memory Barriers

### 8.1. Tại sao cần memory barriers?

ARM architecture cho phép:
- Out-of-order execution
- Write buffering
- Speculative memory access

**Hệ quả:**
- Memory writes có thể không hoàn tất ngay lập tức
- Nếu không có barrier, context save có thể chưa flush ra RAM
- Next task có thể thấy stale data

### 8.2. DSB (Data Synchronization Barrier)

Context switch **phải** có DSB sau khi save context:

```assembly
; Save context
str  r1, [r0, #OFFSET_CONTEXT_SP]
push {r0-r12, lr}

; CRITICAL: DSB to ensure writes complete
dsb

; Now safe to load next task
ldr  sp, [r1, #OFFSET_CONTEXT_SP]
```

**DSB đảm bảo:**
- Tất cả memory writes **trước** DSB đã hoàn tất
- Safe để load next task context

### 8.3. DMB vs DSB vs ISB

ARM có 3 loại barriers:

```
DMB (Data Memory Barrier)     : Đảm bảo memory access ordering
DSB (Data Synchronization)     : Đảm bảo memory operations complete
ISB (Instruction Synchronization): Đảm bảo instruction fetch mới
```

**Context switch chỉ cần DSB:**
- Cần đảm bảo writes complete, không cần ISB (không modify code)
- DSB stronger hơn DMB, đủ cho context switch

---

## 9. Critical Constraints

### 9.1. Atomicity

Context switch **phải** atomic - không bị interrupt giữa chừng.

**Đảm bảo:**
- Context switch chạy trong IRQ handler
- IRQ đã bị disabled (CPSR I bit = 1 trong IRQ mode)
- Không có interrupt khác có thể preempt

**Nếu không atomic:**
- Half-saved context → corruption
- Task switch giữa chừng → crash

### 9.2. Stack alignment

ARM EABI yêu cầu stack **8-byte aligned** tại function call boundaries.

**Context switch phải:**
- Đảm bảo SP luôn 8-byte aligned trước khi call C function
- Stack frame size phải là multiple của 8

**Kiểm tra:**
```assembly
; Ensure SP is 8-byte aligned
and  r0, sp, #7
cmp  r0, #0
bne  alignment_error   ; SP not aligned!
```

### 9.3. Register corruption

Assembly code **phải** preserve registers theo ARM calling convention:

**Caller-saved (có thể corrupt):**
- r0-r3, r12

**Callee-saved (phải preserve):**
- r4-r11, lr

**Context switch đặc biệt:**
- Vì switch tasks, tất cả registers đều bị "corrupt" theo design
- Nhưng saved vào task context, không lost

---

## 10. Stack Pointer Management

### 10.1. Per-task stack

Mỗi task có stack riêng:

```
Task A:
  stack_base = 0x80010000
  stack_size = 4096
  sp (current) = 0x8000F800

Task B:
  stack_base = 0x80014000  
  stack_size = 4096
  sp (current) = 0x80013C00
```

**Context switch phải:**
- Save Task A's SP (0x8000F800)
- Load Task B's SP (0x80013C00)

### 10.2. SP save/restore sequence

```assembly
; Save current task's SP
mov  r2, sp                    ; r2 = current SP
str  r2, [r0, #CONTEXT_SP]     ; current->context.sp = r2

; Load next task's SP  
ldr  r3, [r1, #CONTEXT_SP]     ; r3 = next->context.sp
mov  sp, r3                    ; SP = next's stack
```

### 10.3. Stack overflow detection

**Trong giai đoạn hiện tại:**
- Không có stack overflow detection
- Nếu task push quá nhiều → overwrite memory
- Debug bằng cách check `sp >= (stack_base - stack_size)`

**Future work:**
- Add guard pages
- Check SP bounds trước khi switch
- Panic nếu overflow detected

---

## 11. Return Mechanism

### 11.1. Return từ context_switch()

Context switch **never returns** theo nghĩa thông thường.

**Thay vào đó:**
- Restore next task's PC (từ saved lr)
- Jump đến next task
- Next task tiếp tục chạy như thể không bị gián đoạn

### 11.2. Return address handling

Khi task bị preempt bởi timer interrupt:
- LR (link register) chứa address để return
- LR được saved vào context
- Khi task được schedule lại, LR restored → return đúng vị trí

```assembly
; Timer interrupt occurred here
; ----------------------------- Timer ISR boundary
; LR saved here points to interrupted instruction

; Context switch happens
; ...

; When task scheduled again:
pop {r0-r12, pc}    ; PC = saved LR, return to interrupted point
```

---

## 12. Special Cases

### 12.1. First-time task execution

Khi task mới được tạo, context chưa bao giờ saved.

**Initial context setup (trong task_create):**

```c
task->context.pc = (uint32_t)entry_point;
task->context.sp = (uint32_t)stack_base;
task->context.lr = 0;                      // Never return
task->context.cpsr = 0x13 | (0 << 7);      // SVC mode, IRQ enabled
```

**Lần đầu load context:**
- PC = entry_point → task bắt đầu chạy từ entry_point
- LR = 0 → nếu task return, crash (intended behavior)

### 12.2. Idle task special handling

Idle task chỉ có vòng lặp `wfi()`:

```c
void idle_task(void) {
    while (1) {
        wfi();
    }
}
```

**Context switch vào idle task:**
- Giống như task bình thường
- Idle chạy `wfi()` → CPU sleep cho đến interrupt tiếp theo
- Timer tick → wake up → context switch ra khỏi idle

---

## 13. Testing Strategy

### 13.1. Unit test context switch

**Test 1: Save/Restore integrity**
- Tạo 1 task với known register values
- Manually trigger context switch
- Verify tất cả registers restored đúng

**Test 2: Multiple switches**
- Tạo 2 tasks in vòng lặp counting
- Switch qua lại nhiều lần
- Verify counters đều tăng đúng (không lost updates)

**Test 3: Stack integrity**
- Fill stack với pattern (0xDEADBEEF)
- Context switch nhiều lần
- Verify stack không bị corrupt

### 13.2. Integration test

**Test với timer:**
- Enable timer interrupt
- Tạo 2 tasks: một log "A", một log "B"
- Verify output xen kẽ: "ABABAB..."

---

## 14. Debugging Context Switch Issues

### 14.1. Common bugs

**Bug 1: Register mismatch**
- Symptom: Task crash ngay sau switch
- Cause: Struct field order không match assembly push/pop order
- Fix: Verify struct layout vs assembly code

**Bug 2: Stack corruption**
- Symptom: Random crashes, data corruption
- Cause: SP không được save/restore đúng
- Fix: Log SP values, check alignment

**Bug 3: Mode mismatch**
- Symptom: Prefetch/Data abort exceptions
- Cause: CPSR không được restore đúng, task không ở SVC mode
- Fix: Verify CPSR mode bits

### 14.2. Debug helpers

```assembly
; Log context switch
context_switch:
    ; Breakpoint here để examine registers
    bkpt #0
    
    ; Or call C debug function
    push {r0-r3, lr}
    bl   debug_log_switch
    pop  {r0-r3, lr}
```

---

## 15. Explicit Non-Goals

Context switch contract **explicitly does NOT include**:

- FPU/NEON register save/restore
- Coprocessor state management
- MMU/TLB handling (no virtual memory)
- Cache management (assume write-through, no coherency issues)
- Debugging register state (breakpoints, watchpoints)
- Performance counters
- Trace buffer state

Mọi nội dung nằm ngoài danh sách trên đều **không được giả định**.

---

## 16. Quan hệ với các tài liệu khác

### 16.1. Context switch contract phụ thuộc vào

- `02_task_model.md` - task structure layout
- `04_kernel/02_interrupt_model.md` - context switch xảy ra trong interrupt context

### 16.2. Context switch contract được sử dụng bởi

- `04_scheduler_algorithm.md` - scheduler gọi context_switch()
- `05_timer_integration.md` - timer ISR trigger scheduler → context switch

### 16.3. Context switch contract không thay thế

- Exception handling contract
- Interrupt entry/exit mechanism
- Boot sequence

---

## 17. Tiêu chí thành công

Context switch được coi là thành công khi:

### 17.1. Functional requirements

- Task được save/restore hoàn toàn chính xác
- Task tiếp tục execution đúng vị trí sau khi được scheduled lại
- Không có register corruption
- Stack integrity được giữ nguyên

### 17.2. Code quality requirements

- Assembly code rõ ràng, có comments đầy đủ
- Register usage được document
- Memory barriers đặt đúng vị trí
- Error handling cho edge cases

### 17.3. Performance requirements

- Context switch nhanh (<100 cycles)
- Overhead tối thiểu cho scheduler

---

## 18. Ghi chú thiết kế

- Context switch assembly code là **most critical** trong scheduler
- Một bug nhỏ → kernel crash, very hard to debug
- Phải test thoroughly trước khi integrate vào scheduler
- Stack frame layout và struct layout **must be in sync**
- Memory barriers **không được skip** - dù system có vẻ work without them
- ARM calling convention phải được tuân thủ nếu call C functions
- Comment assembly code rất chi tiết - future maintenance will thank you

---