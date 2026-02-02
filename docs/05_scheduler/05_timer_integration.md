# Timer Integration

## 1. Mục tiêu của tài liệu

Tài liệu này mô tả cách **timer interrupt** được integrate với scheduler để trigger task switching trong RefARM-OS.

Mục tiêu của timer integration:
- Giải thích vai trò của timer trong scheduler subsystem
- Mô tả DMTimer2 configuration
- Fix bug thiếu clock enable trong timer driver hiện tại
- Làm rõ flow từ timer overflow → scheduler → context switch
- Xác định EOI timing để tránh kernel hang

Tài liệu này **không** mô tả:
- Chi tiết hardware của DMTimer (xem training docs)
- Scheduler algorithm (xem `04_scheduler_algorithm.md`)
- Context switch (xem `03_context_switch_contract.md`)

---

## 2. Timer trong Scheduler Subsystem

### 2.1. Vai trò của timer

Timer interrupt đóng vai trò **trigger mechanism** cho scheduler:

**Cooperative multitasking cần timer vì:**
- Tasks không tự nguyện yield
- Cần external event để force context switch
- Timer cung cấp periodic interrupt với interval cố định

**Flow tổng thể:**
```
DMTimer2 overflow (10ms)
    ↓
IRQ exception
    ↓
exception_entry_irq (assembly)
    ↓
irq_dispatch (kernel IRQ core)
    ↓
timer_isr (driver)
    ↓
sched_tick (scheduler)
    ↓
context_switch (assembly)
    ↓
Next task continues
```

### 2.2. Timer tick period

**Time slice = Timer tick period**

Trong RefARM-OS:
- Timer tick = 10ms
- Frequency = 100 Hz
- Mỗi task chạy 10ms trước khi switch

**Lý do chọn 10ms:**
- Đủ dài để giảm context switch overhead
- Đủ ngắn để response time tốt
- Standard choice cho non-real-time OS

---

## 3. DMTimer2 Hardware

### 3.1. Hardware overview

**DMTimer2 properties:**
- Base address: `0x48040000`
- IRQ number: 68
- Input clock: 24 MHz (SYSCLK)
- Mode: Auto-reload, overflow interrupt

### 3.2. Key registers

**TCLR (Timer Control Register):**
```
Bit [0] ST      : Start/Stop timer
Bit [1] AR      : Auto-reload enable
Bit [5] CE      : Compare enable (not used)
Bit [10] SCPWM  : Pulse width modulation (not used)
```

**TLDR (Timer Load Register):**
- Reload value khi timer overflow
- Determines tick period

**TCRR (Timer Counter Register):**
- Current counter value
- Tăng mỗi clock cycle
- Overflow khi TCRR > 0xFFFFFFFF

**IRQSTATUS (Interrupt Status):**
```
Bit [0] MAT_IT_FLAG : Match interrupt (not used)
Bit [1] OVF_IT_FLAG : Overflow interrupt (used)
Bit [2] TCAR_IT_FLAG: Capture interrupt (not used)
```

**IRQENABLE_SET:**
- Set bit để enable interrupt
- Bit [1] = OVF_IT_FLAG

---

## 4. Timer Configuration

### 4.1. Clock enable - CRITICAL FIX

**BUG trong timer driver hiện tại:**
- Timer driver thiếu bước enable clock cho DMTimer2
- Trên real hardware → register access hang

**Pattern từ uart.c (correct):**
```c
// Enable UART0 clock
mmio_write32(CM_PER_UART0_CLKCTRL, 0x2);

// Wait for module to be fully functional
while ((mmio_read32(CM_PER_UART0_CLKCTRL) & 0x30000) != 0);
```

**Phải apply cho DMTimer2:**
```c
#define CM_PER_BASE           0x44E00000
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)

void timer_enable_clock(void) {
    // Enable DMTimer2 module clock
    mmio_write32(CM_PER_TIMER2_CLKCTRL, 0x2);
    
    // Wait for IDLEST bits to indicate module is functional
    // IDLEST is bits [17:16]
    // 0x0 = Fully functional
    while ((mmio_read32(CM_PER_TIMER2_CLKCTRL) & 0x30000) != 0) {
        // Busy wait
    }
}
```

**Gọi trong timer_init() TRƯỚC KHI access timer registers:**
```c
void timer_init(void) {
    // 1. Enable clock FIRST
    timer_enable_clock();
    
    // 2. Now safe to access timer registers
    mmio_write32(TIMER2_TCLR, 0);  // Stop timer
    // ...
}
```

### 4.2. Timer initialization sequence

```c
void timer_init(void) {
    // 1. Enable clock (CRITICAL - must be first)
    timer_enable_clock();
    
    // 2. Stop timer
    mmio_write32(TIMER2_TCLR, 0);
    
    // 3. Set reload value for 10ms @ 24MHz
    // Period = (0xFFFFFFFF - TLDR + 1) / 24MHz
    // For 10ms: count = 24MHz * 0.01 = 240000
    uint32_t reload = 0xFFFFFFFF - 240000 + 1;
    mmio_write32(TIMER2_TLDR, reload);
    mmio_write32(TIMER2_TCRR, reload);  // Initial counter value
    
    // 4. Enable overflow interrupt
    mmio_write32(TIMER2_IRQENABLE_SET, 0x2);  // OVF_IT_FLAG
    
    // 5. Configure timer control
    // AR=1 (auto-reload), ST=0 (stopped, will start later)
    mmio_write32(TIMER2_TCLR, 0x2);
}
```

### 4.3. Timer start

```c
void timer_start(void) {
    uint32_t tclr = mmio_read32(TIMER2_TCLR);
    tclr |= 0x1;  // Set ST bit
    mmio_write32(TIMER2_TCLR, tclr);
}
```

**Gọi sau khi scheduler ready:**
```c
void kernel_main(void) {
    // ... init UART, INTC, IRQ
    
    timer_init();
    
    // Setup scheduler
    sched_init();
    sched_add_task(idle_task, ...);
    sched_add_task(shell_task, ...);
    
    // Start timer (before sched_start)
    timer_start();
    
    // Start scheduler (never returns)
    sched_start();
}
```

---

## 5. Timer ISR Implementation

### 5.1. Current timer ISR (before scheduler)

Hiện tại `timer_isr()` chỉ increment counter:

```c
static volatile uint32_t timer_ticks = 0;

void timer_isr(void) {
    // Clear interrupt at peripheral
    mmio_write32(TIMER2_IRQSTATUS, 0x2);  // Clear OVF_IT_FLAG
    
    // Increment tick counter
    timer_ticks++;
    
    // Log every 100 ticks
    if (timer_ticks % 100 == 0) {
        uart_printf("Timer: %u ticks\n", timer_ticks);
    }
    
    // EOI to INTC (handled by irq_dispatch)
}
```

### 5.2. Modified timer ISR (with scheduler)

```c
static volatile uint32_t timer_ticks = 0;

void timer_isr(void) {
    // 1. Clear interrupt at peripheral (MUST be first)
    mmio_write32(TIMER2_IRQSTATUS, 0x2);
    
    // 2. Increment tick counter (for timekeeping)
    timer_ticks++;
    
    // 3. Call scheduler (triggers context switch)
    sched_tick();
    
    // NOTE: sched_tick() may not return here if context switch happens
    // EOI is handled by irq_dispatch after timer_isr returns
}
```

**CRITICAL: Clear interrupt trước sched_tick()**
- Nếu không clear → interrupt re-asserts ngay lập tức
- ISR được gọi lại vô hạn
- Kernel hang

---

## 6. EOI Timing - CRITICAL

### 6.1. EOI protocol recap

Từ `04_kernel/05_interrupt_infrastructure.md`:

**EOI (End Of Interrupt) MUST be called để:**
- Deassert IRQ line tại INTC
- Cho phép interrupt tiếp theo được process
- Nếu không gọi EOI → IRQ line stuck high → kernel hang

### 6.2. EOI trong timer ISR flow

**Current IRQ dispatch flow:**
```c
void irq_dispatch(void) {
    uint32_t irq_num = intc_get_active_irq();
    
    if (irq_num < 128) {
        // Call registered handler
        if (irq_handlers[irq_num] != NULL) {
            irq_handlers[irq_num]();  // timer_isr() called here
        }
    }
    
    // EOI after handler returns
    intc_eoi();
    dsb();
}
```

**Problem với context switch:**
- `sched_tick()` gọi `context_switch()`
- `context_switch()` never returns (switches to next task)
- `irq_dispatch()` never reaches `intc_eoi()`
- **EOI never called → kernel hang**

### 6.3. Solution: EOI trước sched_tick

**Option 1: Call EOI trong timer_isr**
```c
void timer_isr(void) {
    // Clear peripheral interrupt
    mmio_write32(TIMER2_IRQSTATUS, 0x2);
    
    // EOI immediately (before scheduler)
    intc_eoi();
    dsb();
    
    // Now safe to call scheduler
    sched_tick();  // Never returns
}
```

**Option 2: Modify irq_dispatch**
```c
void irq_dispatch(void) {
    uint32_t irq_num = intc_get_active_irq();
    
    // Special handling for timer IRQ
    if (irq_num == IRQ_TIMER2) {
        // EOI before calling handler
        intc_eoi();
        dsb();
        
        timer_isr();  // May not return
        return;
    }
    
    // Normal path for other IRQs
    if (irq_handlers[irq_num] != NULL) {
        irq_handlers[irq_num]();
    }
    
    intc_eoi();
    dsb();
}
```

**Recommendation: Option 1** (EOI trong timer_isr) đơn giản hơn.

### 6.4. Memory barrier placement

```c
void timer_isr(void) {
    mmio_write32(TIMER2_IRQSTATUS, 0x2);
    
    intc_eoi();
    dsb();  // CRITICAL: ensure EOI write completes
    
    sched_tick();
}
```

**Tại sao DSB critical:**
- EOI write có thể bị buffer
- Nếu context switch xảy ra trước EOI flush → EOI lost
- DSB ensure write hoàn tất

---

## 7. Tick Counter vs Scheduler

### 7.1. Dual purpose của timer

Timer phục vụ 2 mục đích:

**1. Timekeeping (tick counter):**
```c
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t timer_ms_to_ticks(uint32_t ms) {
    return ms / 10;  // 10ms per tick
}
```

**2. Scheduler trigger:**
```c
sched_tick();  // Called every timer interrupt
```

### 7.2. Tick counter use cases

**Use case 1: Uptime tracking**
```c
uint32_t uptime_seconds(void) {
    return timer_ticks / 100;  // 100 ticks = 1 second
}
```

**Use case 2: Shell command**
```c
static int cmd_uptime(int argc, char **argv) {
    uint32_t secs = uptime_seconds();
    uart_printf("Uptime: %u seconds\n", secs);
    return 0;
}
```

**Use case 3: Debugging timestamps**
```c
void debug_log(const char *msg) {
    uart_printf("[%u] %s\n", timer_ticks, msg);
}
```

---

## 8. Integration Checklist

### 8.1. Timer driver modifications

**Files to modify:**
- `drivers/timer.c`
- `include/timer.h`

**Changes:**
1. Add `timer_enable_clock()` function
2. Call `timer_enable_clock()` trong `timer_init()`
3. Modify `timer_isr()` để gọi `sched_tick()`
4. Add EOI call trong `timer_isr()`
5. Export `timer_get_ticks()` cho kernel

### 8.2. Scheduler integration

**Files to modify:**
- `kernel/main.c`

**Changes:**
1. Include `scheduler.h`
2. Gọi `sched_init()` sau IRQ init
3. Gọi `sched_add_task()` cho mỗi task
4. Gọi `timer_start()` trước `sched_start()`
5. Gọi `sched_start()` cuối cùng (never returns)

### 8.3. Testing sequence

**Test 1: Timer ticks without scheduler**
```c
// Don't call sched_start()
// Verify timer_ticks increments
// Verify log output every 100 ticks
```

**Test 2: Single task**
```c
sched_add_task(idle_task, ...);
sched_start();
// Verify no crash (context switch to self)
```

**Test 3: Two tasks**
```c
sched_add_task(idle_task, ...);
sched_add_task(shell_task, ...);
sched_start();
// Verify tasks alternate
// Verify shell responds to input
```

---

## 9. Debugging Timer Integration

### 9.1. Common issues

**Issue 1: Timer không fire**
- Symptom: Không có log từ timer_isr
- Debug: Check clock enable, check IRQENABLE_SET
- Fix: Verify `timer_enable_clock()` được gọi

**Issue 2: Kernel hang sau first tick**
- Symptom: First timer interrupt → hang
- Debug: EOI không được gọi
- Fix: Call `intc_eoi()` trước `sched_tick()`

**Issue 3: Tasks không switch**
- Symptom: Chỉ một task chạy
- Debug: `sched_tick()` có được gọi không?
- Fix: Verify timer_isr gọi sched_tick

**Issue 4: Crash trong context switch**
- Symptom: Data abort exception
- Debug: Stack corruption, register mismatch
- Fix: Verify task structure layout vs assembly

### 9.2. Debug helpers

**Log timer ISR entry:**
```c
void timer_isr(void) {
    static uint32_t isr_count = 0;
    isr_count++;
    
    uart_printf("[ISR %u]\n", isr_count);
    
    // ...
}
```

**Log context switches:**
```c
void sched_tick(void) {
    uart_printf("Switch: %s -> %s\n",
                current_task->name,
                next_task->name);
    
    context_switch(prev, next);
}
```

---

## 10. Performance Considerations

### 10.1. ISR overhead

**Timer ISR execution time:**
- Clear interrupt: ~10 cycles
- Increment counter: ~5 cycles
- EOI: ~20 cycles
- sched_tick: ~20 cycles
- **Total before context switch: ~55 cycles**

**Context switch overhead:**
- ~100 cycles (from `03_context_switch_contract.md`)

**Total interrupt overhead: ~155 cycles @ 1GHz = 155ns**

### 10.2. Interrupt frequency impact

**At 100 Hz (10ms tick):**
- Interrupts per second: 100
- Overhead per second: 155ns × 100 = 15.5µs
- **CPU overhead: 0.00155%**

**Negligible overhead cho scheduler.**

### 10.3. Tick granularity

**Current: 10ms tick**
- Minimum sleep time: 10ms
- Timer resolution: 10ms
- Good enough cho preemptive multitasking

**If need finer granularity:**
- Reduce tick period (e.g., 1ms)
- Trade-off: More interrupt overhead
- Not recommended unless necessary

---

## 11. Timer vs Other Interrupt Sources

### 11.1. Interrupt priority

**INTC priorities (conceptual, no hardware priority):**
1. UART RX (IRQ 72) - user input responsive
2. Timer (IRQ 68) - scheduler tick
3. Other peripherals

**Trong RefARM-OS:**
- Interrupt không nested
- Priority không relevant (FIFO order)

### 11.2. UART interrupt interaction

**UART RX interrupt:**
- Vẫn hoạt động bình thường
- Characters vào ring buffer
- Shell task poll buffer mỗi khi được scheduled

**Flow:**
```
User types character
    ↓
UART RX interrupt
    ↓
Character -> ring buffer
    ↓
Return to current task
    ↓
... (wait for timer tick)
    ↓
Timer interrupt
    ↓
Switch to shell task
    ↓
Shell polls ring buffer
    ↓
Process character
```

---

## 12. Future Enhancements

### 12.1. High-resolution timer

**Nếu cần microsecond resolution:**
- Sử dụng TCRR để read current count
- Calculate elapsed time từ last tick
- Useful cho profiling, benchmarking

```c
uint32_t timer_get_us(void) {
    uint32_t ticks = timer_ticks;
    uint32_t count = mmio_read32(TIMER2_TCRR);
    
    // Calculate microseconds
    // ...
}
```

### 12.2. Watchdog timer

**Nếu cần detect hung tasks:**
- Task phải "pet" watchdog mỗi time slice
- Nếu không pet → watchdog timeout → reset system
- Useful cho production systems

### 12.3. RTC integration

**Nếu cần wall-clock time:**
- Integrate với AM335x RTC module
- Maintain datetime từ timer ticks
- Sync với external time source

---

## 13. Explicit Non-Goals

Timer integration **explicitly does NOT include**:

- Hardware timer configuration nâng cao (PWM, capture, compare)
- Multiple timer usage (chỉ dùng DMTimer2)
- Timer calibration
- Tickless kernel (timer stop khi idle)
- High-precision timekeeping
- RTC integration
- Watchdog timer
- Performance counters
- Timer-based delays (busy wait)

---

## 14. Quan hệ với các tài liệu khác

### 14.1. Timer integration phụ thuộc vào

- `04_kernel/02_interrupt_model.md` - timer ISR tuân theo interrupt model
- `04_kernel/05_interrupt_infrastructure.md` - timer sử dụng IRQ framework
- `04_scheduler_algorithm.md` - timer gọi sched_tick()

### 14.2. Timer integration trigger

- Scheduler subsystem
- Task context switches

### 14.3. Timer integration không thay thế

- Interrupt infrastructure
- EOI protocol
- INTC driver

---

## 15. Tiêu chí thành công

Timer integration thành công khi:

### 15.1. Functional requirements

- Timer interrupt fire đều đặn mỗi 10ms
- `sched_tick()` được gọi mỗi interrupt
- EOI được gọi đúng timing
- Kernel không hang
- Tasks switch đều đặn

### 15.2. Code quality requirements

- Clock enable bug được fix
- EOI placement correct
- Memory barriers đúng vị trí
- Code dễ hiểu, well-commented

### 15.3. Testing requirements

- Test timer alone (no scheduler)
- Test với 1 task
- Test với 2+ tasks
- Long-running stability test (chạy vài phút không crash)

---

## 16. Ghi chú thiết kế

- **Clock enable bug là critical** - không fix sẽ hang trên real hardware
- **EOI timing là critical** - sai timing sẽ hang kernel
- **DSB sau EOI là critical** - không có DSB có thể lost EOI
- Timer ISR phải **minimal** - chỉ clear interrupt, increment counter, gọi scheduler
- Tick counter useful cho debugging - đừng remove
- 10ms tick period là **good default** - không cần thay đổi trừ khi có lý do rõ ràng
- Test thoroughly trước khi integrate scheduler - timer phải stable trước

---