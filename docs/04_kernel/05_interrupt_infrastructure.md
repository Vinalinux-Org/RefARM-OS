# Interrupt Infrastructure Implementation

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả **triển khai cụ thể (implementation)** của interrupt infrastructure trong RefARM-OS.

Mục tiêu:

- Định nghĩa cách IRQ được dispatch từ hardware đến driver ISR.
- Làm rõ cơ chế đăng ký handler và quản lý handler table.
- Mô tả INTC driver contract và EOI sequence.
- Cung cấp checklist để verify implementation.

Tài liệu này **không** thay thế interrupt model (`02_interrupt_model.md`) - đây là implementation detail.

### 1.1. Document Intent

Tài liệu này chứa **cả contract kiến trúc lẫn reference implementation**.

**Phân biệt:**

**CONTRACT (bắt buộc tuân thủ):**
- Được đánh dấu bằng **MUST**, **MUST NOT**, **CRITICAL**.
- Là ràng buộc kiến trúc mà mọi implementation phải tuân theo.
- Ví dụ: "EOI MUST be called", "Handler MUST NOT block".

**REFERENCE IMPLEMENTATION (có thể thay đổi):**
- Là code mẫu minh họa một cách triển khai hợp lệ.
- Không phải là implementation duy nhất.
- Có thể tối ưu hoặc thay thế miễn không vi phạm contract.

**Reader Guidance:**
- Các section được tổ chức theo **vai trò kiến trúc**, không theo thứ tự thực thi.
- Mỗi section sẽ nêu rõ vai trò (Contract / Implementation / Flow) ở phần mở đầu.
- Diagrams minh họa các góc nhìn khác nhau: architecture, contract, sequence.

---

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi của tài liệu này

Interrupt infrastructure implementation áp dụng cho:
- IRQ dispatch mechanism (từ exception entry đến driver ISR).
- Handler registration framework.
- INTC hardware driver.
- IRQ enable/disable trong kernel.

### 2.2. Các chi tiết sau nằm ngoài phạm vi của tài liệu này

- Interrupt model (xem `02_interrupt_model.md`).
- Exception model (xem `03_exception_model.md`).
- Vector table contract (xem `04_vector_table_contract.md`).
- Specific peripheral driver implementation.
- Nested interrupt support (out of scope for current phase).

### 2.3. Implementation này độc lập với

- Specific peripheral drivers (timer, UART, GPIO, etc).
- Scheduler (không có trong current phase).
- Memory allocator (không sử dụng dynamic memory trong IRQ context).

---

## 3. Kiến trúc tổng quan

### 3.1. Component Map

Interrupt infrastructure gồm **4 components độc lập**, mỗi component giải quyết một vấn đề riêng:

| Component | Vai trò | Layer | Tính chất |
|-----------|---------|-------|-----------|
| **IRQ Dispatch Core** | Routing IRQ từ hardware đến driver | Kernel | Runtime (hot path) |
| **Handler Registration** | Quản lý mapping IRQ → Driver ISR | Kernel | Static configuration |
| **INTC Driver** | Hardware abstraction cho AM335x INTC | Kernel | Hardware boundary |
| **IRQ Entry Stub** | CPU exception → C dispatcher | Architecture | Architecture glue |

**Quan hệ:**
- IRQ Entry Stub **calls** Dispatch Core (runtime)
- Dispatch Core **queries** Handler Registration (table lookup)
- Dispatch Core **uses** INTC Driver (EOI, get active IRQ)
- Handler Registration **configured by** Driver init code (static)

Xem diagram: `docs/04_kernel/diagram/irq_architecture_overview.mmd`

### 3.2. Layering

```
[ Peripheral Hardware ]
        ↓
[ AM335x INTC ]
        ↓
[ ARM Core IRQ Line ]
        ↓
[ Vector Table (+0x18) ]
        ↓
[ exception_entry_irq (Assembly) ]
        ↓
[ irq_dispatch() (C) ]
        ↓
[ Registered Handler (Driver ISR) ]
        ↓
[ intc_eoi() ]
        ↓
[ Return to Normal Execution ]
```

### 3.2. Trách nhiệm từng layer

| Layer | Trách nhiệm | Implementation |
|-------|-------------|----------------|
| Peripheral | Generate interrupt signal | Hardware |
| INTC | Prioritize, route to CPU | Hardware + Driver |
| ARM Core | Mode switch, jump to vector | Hardware |
| Vector Table | Route to entry stub | Assembly |
| Entry Stub | Save context, call dispatcher | Assembly |
| Dispatcher | Lookup handler, call ISR | C (kernel) |
| Driver ISR | Handle peripheral event | C (driver) |
| INTC EOI | Acknowledge interrupt | C (driver) |

---

## 4. IRQ Dispatch Mechanism

**Vai trò section này:** Mô tả **RUNTIME FLOW** - cách IRQ được xử lý khi xảy ra.

**Contract vs Implementation:**
- **CONTRACT:** EOI phải được gọi, spurious IRQ phải được handle
- **IMPLEMENTATION:** Cách organize dispatch function, data structure

### 4.1. Dispatch Flow

**High-level overview:** `docs/04_kernel/diagram/irq_architecture_overview.mmd`

**Dispatch contract:** `docs/04_kernel/diagram/irq_dispatch_contract.mmd`

**Flow ngắn gọn:**
1. Exception entry stub gọi `irq_dispatch()`
2. Query INTC để lấy active IRQ number
3. Validate IRQ number (0-127)
4. Lookup handler trong registration table
5. Call driver ISR (nếu registered)
6. **CRITICAL:** Send EOI to INTC
7. Return về exception entry stub

### 4.2. Handler Registration Table

**Data Structure:**

```c
#define MAX_IRQS 128  /* AM335x supports 128 interrupt sources */

struct irq_desc {
    irq_handler_t handler;  /* Function pointer */
    void *data;             /* Driver-specific data */
    uint32_t count;         /* Call count (statistics) */
};

static struct irq_desc irq_table[MAX_IRQS];
```

**Invariants:**
- `irq_table` được khởi tạo toàn bộ = 0 khi kernel boot.
- `handler == NULL` nghĩa là IRQ chưa được đăng ký.
- `handler != NULL` nghĩa là IRQ đã có owner.

### 4.3. Dispatch Algorithm

**Reference implementation (tuân thủ contract):**

```c
void irq_dispatch(struct exception_context *ctx)
{
    /* Step 1: Query INTC for active IRQ number */
    uint32_t irq_num = intc_get_active_irq();
    
    /* Step 2: Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        uart_printf("ERROR: Invalid IRQ number %u\n", irq_num);
        intc_eoi(irq_num);  /* CONTRACT: EOI MUST be called even for invalid IRQ */
        return;
    }
    
    /* Step 3: Check if handler is registered */
    if (irq_table[irq_num].handler == NULL) {
        uart_printf("WARNING: Unhandled IRQ %u\n", irq_num);
        intc_eoi(irq_num);  /* CONTRACT: EOI MUST be called even for unhandled IRQ */
        return;
    }
    
    /* Step 4: Call registered handler */
    irq_table[irq_num].handler(irq_table[irq_num].data);
    
    /* Step 5: Update statistics */
    irq_table[irq_num].count++;
    
    /* Step 6: Send EOI to INTC (CRITICAL CONTRACT) */
    intc_eoi(irq_num);
}
```

**CONTRACT Requirements:**
- **MUST** call EOI for ALL code paths (valid, spurious, unhandled)
- **MUST** call EOI AFTER handler execution
- **MUST NOT** return without calling EOI

### 4.4. Spurious IRQ Handling

**Spurious IRQ là gì:**
- IRQ number không hợp lệ (>= 128).
- IRQ hợp lệ nhưng không có handler đăng ký.
- Hardware glitch hoặc race condition.

**Xử lý:**
- Log warning message.
- Gọi EOI để clear INTC state.
- Return về normal execution.
- **Không** halt kernel (spurious IRQ không phải fatal error).

---

## 5. Handler Registration Contract

**Vai trò section này:** Định nghĩa **STATIC CONTRACT** giữa kernel IRQ core và driver.

**Contract vs Implementation:**
- **CONTRACT:** Handler function signature, registration rules, handler constraints
- **IMPLEMENTATION:** Handler table structure (có thể dùng hash table, linked list, etc)

**Tại sao cần component này:**
- Kernel cần biết "khi IRQ X xảy ra, gọi function nào?"
- Driver cần cách đăng ký handler mà không phụ thuộc vào hardware detail

### 5.1. Handler Registration API

```c
/**
 * Register IRQ handler
 * @param irq_num   IRQ number (0-127)
 * @param handler   Handler function pointer
 * @param data      Driver-specific data (passed to handler)
 * @return 0 on success, -1 on error
 */
int irq_register_handler(uint32_t irq_num, irq_handler_t handler, void *data);

/**
 * Unregister IRQ handler
 * @param irq_num   IRQ number (0-127)
 */
void irq_unregister_handler(uint32_t irq_num);
```

### 5.2. Handler Function Signature

```c
typedef void (*irq_handler_t)(void *data);
```

**CONTRACT:**
- Handler **MUST** return void.
- Handler **MUST NOT** block.
- Handler **MUST NOT** call kernel services phức tạp.
- Handler **SHOULD** xử lý nhanh nhất có thể.

### 5.3. Handler Constraints

**Handler ĐƯỢC PHÉP (permitted):**
- Đọc/ghi peripheral registers (MMIO).
- Clear interrupt flag ở peripheral.
- Update shared data structures (nếu đã sync đúng cách).
- Gọi `uart_printf()` (chỉ dùng cho debug, không production).

**Handler KHÔNG ĐƯỢC PHÉP (CONTRACT violation):**
- Gọi blocking operations.
- Cấp phát bộ nhớ động (`malloc`, `kmalloc`).
- Thực hiện I/O phức tạp.
- Giả định interrupt nesting (current phase không support).
- Enable interrupt trong handler (IRQ đang masked).

**Rationale:** IRQ context là atomic, deterministic. Vi phạm constraints trên có thể gây deadlock, race condition, hoặc undefined behavior.

### 5.4. Registration Rules

**Quy tắc:**
1. **Mutual exclusion:** Một IRQ chỉ có một handler tại một thời điểm.
2. **Register trước enable:** Driver phải register handler trước khi enable IRQ.
3. **Disable trước unregister:** Driver phải disable IRQ trước khi unregister handler.
4. **No dynamic registration trong IRQ context:** Không được register/unregister handler từ trong IRQ handler.

**Implementation:**

```c
int irq_register_handler(uint32_t irq_num, irq_handler_t handler, void *data)
{
    /* Validate IRQ number */
    if (irq_num >= MAX_IRQS) {
        return -1;
    }
    
    /* Check if already registered */
    if (irq_table[irq_num].handler != NULL) {
        uart_printf("ERROR: IRQ %u already registered\n", irq_num);
        return -1;
    }
    
    /* Register handler */
    irq_table[irq_num].handler = handler;
    irq_table[irq_num].data = data;
    irq_table[irq_num].count = 0;
    
    return 0;
}

void irq_unregister_handler(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQS) {
        return;
    }
    
    /* Clear registration */
    irq_table[irq_num].handler = NULL;
    irq_table[irq_num].data = NULL;
    /* Keep count for statistics */
}
```

---

## 6. INTC Driver Contract

**Vai trò section này:** Định nghĩa **HARDWARE BOUNDARY** - ranh giới giữa kernel và AM335x INTC.

**Contract vs Implementation:**
- **CONTRACT:** EOI sequence timing, API behavior (get_active_irq, enable, disable)
- **IMPLEMENTATION:** Register addresses, bitfield manipulation (có thể optimize)

**Tại sao cần component này:**
- Ẩn hardware detail khỏi IRQ dispatch core
- Cho phép kernel portable (thay INTC khác chỉ cần thay driver này)
- Enforce EOI contract ở một chỗ duy nhất

### 6.1. INTC Hardware Overview

AM335x INTC (Interrupt Controller) features:
- 128 interrupt sources (IRQ 0-127).
- Priority-based arbitration (0-63, lower = higher priority).
- Programmable interrupt routing (IRQ/FIQ).
- Software-triggered interrupts.

**Base Address:** `0x48200000`

### 6.2. INTC Register Map

| Register | Offset | Description |
|----------|--------|-------------|
| SYSCONFIG | 0x10 | System configuration |
| SYSSTATUS | 0x14 | System status |
| SIR_IRQ | 0x40 | Active IRQ number (0-127) |
| SIR_FIQ | 0x44 | Active FIQ number |
| CONTROL | 0x48 | New IRQ agreement |
| MIR_CLEAR(n) | 0x88 + n*0x20 | Enable IRQ (clear mask) |
| MIR_SET(n) | 0x8C + n*0x20 | Disable IRQ (set mask) |
| ISR_SET(n) | 0x90 + n*0x20 | Software trigger |
| ISR_CLEAR(n) | 0x94 + n*0x20 | Software clear |

**Note:** INTC có 4 banks (n = 0, 1, 2, 3), mỗi bank quản lý 32 IRQs.

### 6.3. INTC Driver API

```c
/**
 * Initialize INTC
 * Must be called before any IRQ usage
 */
void intc_init(void);

/**
 * Get active IRQ number
 * @return IRQ number (0-127), or 128 if spurious
 */
uint32_t intc_get_active_irq(void);

/**
 * End-of-Interrupt sequence
 * @param irq_num IRQ number (not used by hardware, for logging only)
 * 
 * CRITICAL: Must be called after handling IRQ
 * Without EOI, INTC will not de-assert IRQ line
 */
void intc_eoi(uint32_t irq_num);

/**
 * Enable specific IRQ
 * @param irq_num IRQ number (0-127)
 */
void intc_enable_irq(uint32_t irq_num);

/**
 * Disable specific IRQ
 * @param irq_num IRQ number (0-127)
 */
void intc_disable_irq(uint32_t irq_num);

/**
 * Set IRQ priority
 * @param irq_num IRQ number (0-127)
 * @param priority Priority (0-63, lower = higher)
 * 
 * Optional: Current implementation may not use this
 */
void intc_set_priority(uint32_t irq_num, uint32_t priority);
```

### 6.4. EOI Sequence (CRITICAL)

**EOI Timing Contract:** Xem diagram `docs/04_kernel/diagram/eoi_sequence.mmd`

**End-of-Interrupt Timing:**

```
[ IRQ Handler Entry ]
        ↓
[ Read INTC_SIR_IRQ ]
        ↓
[ Call Driver ISR ]
        ↓
[ Write INTC_CONTROL = 0x1 ]  ← EOI MUST HAPPEN HERE
        ↓
[ Return to Normal Execution ]
```

**Why EOI is CRITICAL:**
- INTC sẽ **không** de-assert IRQ line cho đến khi nhận được EOI.
- Không có EOI → IRQ line vẫn active → CPU sẽ nhảy vào IRQ handler lại ngay khi return.
- Result: Infinite IRQ loop → kernel hang.

**EOI Implementation:**

```c
void intc_eoi(uint32_t irq_num)
{
    /* 
     * AM335x INTC EOI Protocol (TRM Section 6.5.3):
     * Write 0x1 to CONTROL register.
     * This signals "New IRQ Agreement" to INTC.
     * 
     * CONTRACT: This function MUST be called after handling IRQ.
     * Without EOI, INTC will NOT de-assert IRQ line.
     */
    mmio_write32(INTC_BASE + INTC_CONTROL, 0x01);
    
    /* 
     * Hardware action after EOI:
     * - INTC de-asserts IRQ line to CPU
     * - Next highest priority interrupt (if any) becomes active
     * - SIR_IRQ register updates to next IRQ number
     */
}
```

**CONTRACT Requirements:**
- **MUST** be called after driver ISR completes
- **MUST** be called even for spurious/unhandled IRQ
- **MUST NOT** be called before driver ISR (race condition)

**Note:** Tham số `irq_num` không được sử dụng bởi hardware, chỉ để logging/debugging.

### 6.5. INTC Initialization Sequence

```c
void intc_init(void)
{
    /* Step 1: Soft reset INTC */
    mmio_write32(INTC_BASE + INTC_SYSCONFIG, 0x02);
    
    /* Wait for reset complete (SYSSTATUS bit 0 = 1) */
    while (!(mmio_read32(INTC_BASE + INTC_SYSSTATUS) & 0x01))
        ;
    
    /* Step 2: Disable all IRQs (set all mask bits) */
    for (int bank = 0; bank < 4; bank++) {
        mmio_write32(INTC_BASE + INTC_MIR_SET(bank), 0xFFFFFFFF);
    }
    
    /* Step 3: Configure priority (optional, default is fine) */
    /* All IRQs default to priority 0 */
    
    /* Step 4: Enable new IRQ agreement protocol */
    mmio_write32(INTC_BASE + INTC_CONTROL, 0x01);
    
    /* INTC is now ready to accept IRQ registrations */
}
```

### 6.6. Enable/Disable IRQ Implementation

**Enable IRQ (Clear Mask):**

```c
void intc_enable_irq(uint32_t irq_num)
{
    if (irq_num >= 128) {
        return;
    }
    
    /* Calculate bank and bit position */
    uint32_t bank = irq_num / 32;   /* 0-3 */
    uint32_t bit = irq_num % 32;    /* 0-31 */
    
    /* Clear mask bit (enable interrupt) */
    mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}
```

**Disable IRQ (Set Mask):**

```c
void intc_disable_irq(uint32_t irq_num)
{
    if (irq_num >= 128) {
        return;
    }
    
    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;
    
    /* Set mask bit (disable interrupt) */
    mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}
```

**Get Active IRQ:**

```c
uint32_t intc_get_active_irq(void)
{
    /* Read SIR_IRQ register */
    uint32_t sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
    
    /* Extract IRQ number (bits 6:0) */
    uint32_t irq_num = sir & 0x7F;
    
    /* Check spurious bit (bit 7) */
    if (sir & 0x80) {
        return 128;  /* Spurious IRQ */
    }
    
    return irq_num;
}
```

---

## 7. IRQ Entry Stub (Assembly)

**Vai trò section này:** Định nghĩa **ARCHITECTURE GLUE** - bridge giữa ARM CPU và C code.

**Contract vs Implementation:**
- **CONTRACT:** LR adjustment (-4), context save order, SPSR restore
- **IMPLEMENTATION:** Instruction choice (STMFD vs PUSH), register allocation

**Tại sao cần component này:**
- ARM CPU không thể gọi C function trực tiếp khi exception xảy ra
- Phải save context trước, gọi C, restore context sau
- Phải tuân theo ARM calling convention và exception return protocol

### 7.1. Entry Stub Requirements

Entry stub (`exception_entry_irq`) phải:
1. Adjust LR (return address) `-4` theo ARM IRQ convention.
2. Save all general-purpose registers (`r0-r12`).
3. Save LR (return address).
4. Save SPSR (pre-exception CPSR).
5. Call `irq_dispatch()` với pointer tới context.
6. Restore registers.
7. Return với `subs pc, lr, #4` (hoặc `ldmfd ... pc^` nếu đã adjust LR).

### 7.2. Implementation

**File:** `src/arch/arm/exceptions/exception_entry.S`

```asm
/*
 * IRQ Exception Entry Stub
 * Entry point: exception_entry_irq
 * Called from: vector_irq (vector table)
 */
exception_entry_irq:
    /*
     * CRITICAL: Adjust LR first
     * ARM IRQ LR points to PC+4, but we want to return to interrupted instruction
     * Per ARM Architecture Reference Manual: IRQ return needs LR-4
     */
    sub lr, lr, #4
    
    /*
     * Save context (MUST preserve r0 first, then rest)
     * Order matches struct exception_context in exception.h
     */
    stmfd sp!, {r0-r12, lr}     /* Save r0-r12, LR */
    mrs r0, spsr                 /* Get SPSR */
    stmfd sp!, {r0}              /* Save SPSR (SP now points to full context) */
    
    /*
     * Call C dispatcher
     * r0 = pointer to exception_context (current SP)
     */
    mov r0, sp
    bl irq_dispatch              /* Call kernel IRQ dispatcher */
    
    /*
     * Restore context
     */
    ldmfd sp!, {r0}              /* Restore SPSR */
    msr spsr_cxsf, r0            /* Write back to SPSR */
    
    /*
     * Return from IRQ
     * ldmfd with ^ suffix: restores CPSR from SPSR automatically
     */
    ldmfd sp!, {r0-r12, pc}^     /* Restore registers and return */
```

**Verification Points:**
- LR adjustment: `-4` (theo vector table contract Table 8.1).
- Context save order: `r0-r12, lr` first, then `SPSR` (preserves r0 correctly).
- Return instruction: `ldmfd ... pc^` (restores CPSR from SPSR).

---

## 8. Kernel IRQ Enable/Disable

### 8.1. CPSR Bit Definitions

**CPSR I-bit (bit 7):**
- `I = 1`: IRQ masked (CPU ignores IRQ line)
- `I = 0`: IRQ enabled (CPU responds to IRQ)

**CPSR F-bit (bit 6):**
- `F = 1`: FIQ masked
- `F = 0`: FIQ enabled (not used in current phase)

### 8.2. IRQ Enable/Disable API

**File:** `src/arch/arm/include/cpu.h`

```c
/* CPSR bit masks */
#define CPSR_IRQ_BIT    (1 << 7)
#define CPSR_FIQ_BIT    (1 << 6)

/**
 * Enable IRQ (clear I bit in CPSR)
 */
static inline void irq_enable(void)
{
    uint32_t cpsr;
    asm volatile(
        "mrs %0, cpsr\n"           /* Read CPSR */
        "bic %0, %0, %1\n"         /* Clear I bit */
        "msr cpsr_c, %0"           /* Write back CPSR */
        : "=r"(cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
}

/**
 * Disable IRQ (set I bit in CPSR)
 */
static inline void irq_disable(void)
{
    uint32_t cpsr;
    asm volatile(
        "mrs %0, cpsr\n"
        "orr %0, %0, %1\n"
        "msr cpsr_c, %0"
        : "=r"(cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
}

/**
 * Check if IRQ is enabled
 * @return 1 if enabled, 0 if disabled
 */
static inline int irq_is_enabled(void)
{
    uint32_t cpsr;
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    return !(cpsr & CPSR_IRQ_BIT);
}
```

### 8.3. Kernel Initialization Sequence

**File:** `src/kernel/main.c`

```c
void kernel_main(void)
{
    /* 1. Critical early init */
    watchdog_disable();
    uart_init();
    
    uart_printf("Kernel starting...\n");
    
    /* 2. Initialize interrupt infrastructure */
    uart_printf("Initializing INTC...\n");
    intc_init();
    
    uart_printf("Initializing IRQ framework...\n");
    irq_init();  /* Clear handler table */
    
    /* 3. Register peripheral handlers (example) */
    /* timer_init(); */  /* Timer will register its handler */
    
    /* 4. Enable IRQ in CPSR */
    uart_printf("Enabling IRQ...\n");
    irq_enable();
    
    uart_printf("Kernel ready. Waiting for interrupts...\n");
    
    /* 5. Main loop (WFI saves power) */
    while (1) {
        asm volatile("wfi");  /* Wait for interrupt */
    }
}
```

**Critical Order:**
1. `intc_init()` - setup hardware controller
2. `irq_init()` - clear software handler table
3. Register handlers (optional, can be done later by drivers)
4. `irq_enable()` - unmask IRQ in CPSR

**Không được:**
- Enable IRQ trước khi `intc_init()` - INTC chưa sẵn sàng
- Enable IRQ trước khi `irq_init()` - handler table chưa được clear

---

## 9. Testing Strategy

### 9.1. Unit Tests

**Test 1: Spurious IRQ**
```c
/* Manually trigger software interrupt without handler */
intc_software_trigger(10);  /* IRQ 10 */
/* Expected: Log "Unhandled IRQ 10", EOI, return */
```

**Test 2: Handler Registration**
```c
/* Register handler */
int ret = irq_register_handler(68, timer_handler, NULL);
assert(ret == 0);

/* Try to register again (should fail) */
ret = irq_register_handler(68, timer_handler, NULL);
assert(ret == -1);

/* Unregister */
irq_unregister_handler(68);

/* Re-register (should succeed) */
ret = irq_register_handler(68, timer_handler, NULL);
assert(ret == 0);
```

**Test 3: INTC Enable/Disable**
```c
/* Disable IRQ */
intc_disable_irq(68);
/* Trigger (should not fire) */
intc_software_trigger(68);

/* Enable IRQ */
intc_enable_irq(68);
/* Trigger (should fire if handler registered) */
intc_software_trigger(68);
```

### 9.2. Integration Test: Timer Interrupt

**Goal:** Verify end-to-end IRQ flow với real peripheral.

**Test Driver:** DMTimer2

```c
/* Timer IRQ handler */
static volatile uint32_t timer_ticks = 0;

void timer_irq_handler(void *data)
{
    /* Clear interrupt status at peripheral */
    mmio_write32(DMTIMER2_BASE + IRQSTATUS, 0x02);
    
    /* Update tick count */
    timer_ticks++;
    
    /* Log every 100 ticks */
    if (timer_ticks % 100 == 0) {
        uart_printf("Timer tick: %u\n", timer_ticks);
    }
}

/* Timer initialization */
void timer_init(void)
{
    /* 1. Stop timer */
    mmio_write32(DMTIMER2_BASE + TCLR, 0x00);
    
    /* 2. Configure load value (overflow after 64K counts) */
    mmio_write32(DMTIMER2_BASE + TLDR, 0xFFFF0000);
    mmio_write32(DMTIMER2_BASE + TCRR, 0xFFFF0000);
    
    /* 3. Enable overflow interrupt at peripheral */
    mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, 0x02);
    
    /* 4. Register IRQ handler with kernel */
    irq_register_handler(IRQ_TIMER2, timer_irq_handler, NULL);
    
    /* 5. Enable IRQ at INTC */
    intc_enable_irq(IRQ_TIMER2);
    
    /* 6. Start timer (auto-reload + enable) */
    mmio_write32(DMTIMER2_BASE + TCLR, 0x03);
    
    uart_printf("Timer initialized (IRQ %d)\n", IRQ_TIMER2);
}
```

**Test in kernel_main:**
```c
void kernel_main(void)
{
    /* ... init code ... */
    
    timer_init();      /* Register timer handler */
    irq_enable();      /* Enable IRQ in CPSR */
    
    uart_printf("Waiting for timer interrupts...\n");
    
    while (1) {
        asm volatile("wfi");
    }
}
```

**Expected Output:**
```
Timer initialized (IRQ 68)
Waiting for timer interrupts...
Timer tick: 100
Timer tick: 200
Timer tick: 300
...
```

### 9.3. Verification Checklist

**Pre-implementation:**
- [ ] Vector table has `vector_irq` entry at offset +0x18
- [ ] IRQ stack initialized in `reset.S`
- [ ] VBAR configured to point to vector table

**IRQ Entry:**
- [ ] `exception_entry_irq` adjusts LR correctly (sub lr, lr, #4)
- [ ] Context saved in correct order (r0-r12, lr, spsr)
- [ ] `irq_dispatch()` called with context pointer
- [ ] Context restored correctly
- [ ] Return instruction uses SPSR restore (ldmfd ... pc^)

**Dispatch Framework:**
- [ ] `irq_table` initialized to zero
- [ ] Spurious IRQ handled without crash
- [ ] Unhandled IRQ logged and EOI'd
- [ ] Handler called with correct data pointer
- [ ] Statistics counter incremented

**INTC Driver:**
- [ ] INTC soft reset completed
- [ ] All IRQs initially disabled
- [ ] EOI sequence correct (write CONTROL = 0x1)
- [ ] Enable/disable IRQ updates correct MIR register
- [ ] Active IRQ number read correctly from SIR_IRQ

**Kernel Integration:**
- [ ] `intc_init()` called before IRQ enable
- [ ] `irq_init()` called before IRQ enable
- [ ] IRQ enabled in CPSR at correct time
- [ ] Timer interrupts fire and handled
- [ ] No crashes or hangs

---

## 10. Known Limitations

### 10.1. Current Phase Limitations

**Không support:**
- Nested interrupts (IRQ masked trong IRQ handler)
- Interrupt priority preemption
- Softirq / bottom-half / deferred work
- IRQ thread
- IRQ affinity (SMP)

**Rationale:** Giữ implementation đơn giản và deterministic trong giai đoạn đầu.

### 10.2. Performance Considerations

**Overhead:**
- Context save/restore: ~30 instructions
- Dispatch lookup: O(1) array access
- INTC register access: ~3-5 bus cycles per access

**Latency:**
- IRQ entry latency: ~100-200 CPU cycles (estimate)
- Handler execution: Driver-dependent
- EOI overhead: ~10 CPU cycles

**Scalability:**
- Handler table: Fixed 128 entries (512 bytes)
- No dynamic allocation
- No handler chaining (one handler per IRQ)

---

## 11. Future Enhancements

### 11.1. Possible Extensions

**Nested Interrupt Support:**
- Re-enable IRQ trong handler (clear I bit)
- Separate interrupt stack management
- Priority-based preemption

**Deferred Work:**
- Softirq mechanism (Linux-style)
- Workqueue for long-running tasks
- Separate context for deferred handlers

**Advanced Features:**
- IRQ handler chaining (multiple handlers per IRQ)
- Dynamic handler registration from interrupt context
- IRQ statistics and profiling
- IRQ balancing (for SMP)

### 11.2. Enhancement Considerations

Mọi enhancement phải:
- Maintain backward compatibility với current API
- Update docs trước khi implement
- Verify không làm tăng complexity không cần thiết
- Test thoroughly với existing drivers

---

## 12. Quan hệ với các tài liệu khác

### Tài liệu này triển khai:
- `02_interrupt_model.md` (interrupt model)
- `03_exception_model.md` (IRQ là exception type)
- `04_vector_table_contract.md` (IRQ vector entry)

### Tài liệu này phụ thuộc vào:
- `01_execution_model.md` (steady phase)
- `02_boot/boot_contract.md` (CPU state)

### Tài liệu này không thay thế:
- Peripheral driver documentation (timer, UART, etc)
- INTC hardware manual (AM335x TRM Chapter 6)

Mọi thay đổi làm ảnh hưởng đến **IRQ dispatch flow** hoặc **handler registration contract** phải được phản ánh đồng bộ trong implementation code và test cases.

---

## 13. Ghi chú thiết kế

- IRQ infrastructure phải **robust** - spurious IRQ không được crash kernel.
- EOI sequence phải **chính xác** - thiếu EOI sẽ khiến kernel hang.
- Handler registration phải **đơn giản** - không dùng dynamic memory, không lock.
- Dispatch phải **nhanh** - O(1) lookup, minimal overhead.
- Testing phải **comprehensive** - unit test + integration test + real hardware test.

---

## 14. Implementation Checklist

Khi implement interrupt infrastructure, verify:

**Core Framework:**
- [ ] `irq_dispatch()` implemented
- [ ] `irq_table` defined and initialized
- [ ] `irq_register_handler()` implemented
- [ ] `irq_unregister_handler()` implemented
- [ ] Spurious IRQ handling correct

**INTC Driver:**
- [ ] `intc_init()` implemented
- [ ] `intc_get_active_irq()` implemented
- [ ] `intc_eoi()` implemented
- [ ] `intc_enable_irq()` implemented
- [ ] `intc_disable_irq()` implemented

**Assembly Entry:**
- [ ] `exception_entry_irq` implemented
- [ ] LR adjustment correct
- [ ] Context save/restore correct
- [ ] Call to `irq_dispatch()` correct

**Kernel Integration:**
- [ ] `irq_enable()` / `irq_disable()` implemented
- [ ] `irq_init()` called in `kernel_main()`
- [ ] `intc_init()` called before IRQ enable
- [ ] IRQ enabled at correct point

**Testing:**
- [ ] Spurious IRQ test passes
- [ ] Handler registration test passes
- [ ] Timer interrupt test passes
- [ ] No kernel crashes or hangs
- [ ] EOI sequence verified

**Documentation:**
- [ ] This document completed
- [ ] Diagram created (`irq_dispatch_detail.mmd`)
- [ ] Link from `02_interrupt_model.md` added
- [ ] Code comments updated