# Vector Table Contract

## 1. Mục tiêu của tài liệu

Tài liệu này định nghĩa **vector table contract** - hợp đồng giữa ARM CPU hardware và RefARM-OS kernel.

Mục tiêu của vector table contract là:

- Xác định layout cụ thể của vector table.
- Định nghĩa entry point requirements cho từng exception.
- Làm rõ CPU state khi vào exception entry.
- Định nghĩa stack requirements cho từng exception mode.
- Là **contract** cho cả kernel và future contributors.

Tài liệu này **không** mô tả implementation detail của exception entry code.

---

## 2. Phạm vi và ranh giới

### 2.1. Phạm vi của vector table contract

Vector table contract áp dụng cho:
- Physical layout của vector table trong memory.
- Entry point naming và conventions.
- CPU state guarantees khi entry code được gọi.

### 2.2. Các chi tiết sau nằm ngoài phạm vi của tài liệu này

- Assembly implementation của exception entry.
- Context save/restore mechanism chi tiết.
- Exception handler C function signatures.
- Interrupt controller programming.

### 2.3. Vector table contract này độc lập với

- Bootloader cụ thể.
- Linker script implementation details.
- Specific compiler or toolchain.

---

## 3. ARM Vector Table Standard

### 3.1. Vector Table Definition

ARM architecture định nghĩa vector table là:
- Một array of **8 entries**.
- Mỗi entry là một **branch instruction** hoặc **data word** (implementation-defined).
- CPU hardware sẽ jump đến vector table khi exception xảy ra.

### 3.2. Vector Base Address Register (VBAR)

**ARM Cortex-A8 (AM335x) supports:**
- **Programmable vector base** via VBAR (CP15, c12, c0, 0)
- VBAR can point to any 32-byte aligned address

**RefARM-OS uses:**
- **VBAR-based vector table**
- Vector table placed in kernel image (example: `0x80000000`)
- VBAR configured during boot (`entry.S`)

**Lý do:**
- AM335x Boot ROM owns low memory region
- Flexible kernel placement
- Standard ARM Cortex-A practice

---

## 4. Vector Table Layout

### 4.1. Offset-Based Layout (ARM Standard)

```
Offset     | Exception Type          | Entry Point Symbol
-----------|-------------------------|-------------------
+0x00      | Reset                   | (Boot ROM on AM335x)
+0x04      | Undefined Instruction   | vector_undef
+0x08      | Software Interrupt      | vector_svc
+0x0C      | Prefetch Abort          | vector_pabt
+0x10      | Data Abort              | vector_dabt
+0x14      | Reserved                | (not used)
+0x18      | IRQ                     | vector_irq
+0x1C      | FIQ                     | vector_fiq
```

**Lưu ý quan trọng:**
- Các offset này là **cố định theo ARM architecture**
- Địa chỉ thực tế = VBAR + Offset
- Ví dụ: Nếu VBAR = 0x80000000, thì IRQ entry = 0x80000018

### 4.2. Entry Point Responsibilities

Mỗi entry point có trách nhiệm:
- **Routing:** Chuyển điều khiển từ vector offset đến exception entry code.
- **Minimal:** Không thực hiện logic phức tạp.

**Typical implementation:**
```asm
; Tại VBAR + 0x04:
vector_undef:
    b   exception_entry_undef

; Tại VBAR + 0x08:
vector_svc:
    b   exception_entry_svc

; etc.
```

---

## 5. CPU State on Exception Entry

### 5.1. CPU Mode Transitions

Khi exception xảy ra, CPU tự động:

| Exception Type | New CPU Mode | Mode Bits (M[4:0]) |
|----------------|--------------|-------------------|
| Reset          | SVC          | 0b10011           |
| Undefined      | UND          | 0b11011           |
| SVC            | SVC          | 0b10011           |
| Prefetch Abort | ABT          | 0b10111           |
| Data Abort     | ABT          | 0b10111           |
| IRQ            | IRQ          | 0b10010           |
| FIQ            | FIQ          | 0b10001           |

### 5.2. Register State on Entry

Khi CPU jumps đến vector entry:

**Preserved by hardware:**
- `LR_<mode>`: Contains return address
  - For most exceptions: `LR = PC + 4`
  - For Prefetch Abort: `LR = PC + 4`
  - For Data Abort: `LR = PC + 8`
- `SPSR_<mode>`: Contains CPSR before exception
- `SP_<mode>`: Exception mode stack pointer

**Modified by hardware:**
- `PC`: Set to `VBAR + <exception offset>`
  - Example: For IRQ, `PC = VBAR + 0x18`
- `CPSR`: Mode bits changed, IRQ/FIQ may be disabled

**Unchanged:**
- General purpose registers `r0-r12`
- User mode `SP_usr`, `LR_usr`

### 5.3. Interrupt Masking on Entry

| Exception | I bit (IRQ mask) | F bit (FIQ mask) |
|-----------|-----------------|------------------|
| Reset     | 1 (disabled)    | 1 (disabled)     |
| Undefined | 1 (disabled)    | unchanged        |
| SVC       | unchanged       | unchanged        |
| Prefetch  | 1 (disabled)    | unchanged        |
| Data      | 1 (disabled)    | unchanged        |
| IRQ       | 1 (disabled)    | unchanged        |
| FIQ       | 1 (disabled)    | 1 (disabled)     |

**Contract:** Exception entry code **must not** assume interrupts are enabled.

---

## 6. Stack Requirements

### 6.1. Stack per Exception Mode

Mỗi exception mode **must** có dedicated stack:

| Mode | Stack Symbol    | Purpose                |
|------|----------------|------------------------|
| UND  | `__und_stack_end`  | Undefined Instruction  |
| ABT  | `__abt_stack_end`  | Data/Prefetch Abort    |
| IRQ  | `__irq_stack_end`  | IRQ handler            |
| FIQ  | `__fiq_stack_end`  | FIQ handler (reserved) |
| SVC  | `__svc_stack_end`  | Main kernel stack      |

### 6.2. Stack Size Requirements

**Minimum stack sizes:**
- **UND stack:** 512 bytes (handlers are simple)
- **ABT stack:** 512 bytes (handlers are simple)
- **IRQ stack:** 1024 bytes (may nest in future)
- **FIQ stack:** 512 bytes (not used currently)
- **SVC stack:** 4096 bytes (main kernel stack)

**Contract:** Exception entry code **must not** overflow stack.

### 6.3. Stack Direction

ARM stacks grow **downward** (full descending):
- Stack pointer points to **last filled** location.
- Push: SP decrements **before** write.
- Pop: Read, then SP increments.

**Linker script must provide:**
```
__und_stack_base = <base_address>;
__und_stack_end = __und_stack_base + 512;
```

---

## 7. Exception Entry Point Contract

### 7.1. Entry Point Requirements

Mỗi exception entry point **must:**
1. Save all general-purpose registers (r0-r12).
2. Save LR (return address).
3. Save SPSR (saved CPSR).
4. Setup stack frame (if needed for C code).
5. Call C exception handler.
6. Restore all registers.
7. Exception return (using appropriate instruction).

### 7.2. Entry Point Naming Convention

```
vector_<type>           - Vector table entry (may be just a branch)
exception_entry_<type>  - Actual exception entry code
<type>_handler          - C function handler
```

**Example:**
```asm
; Vector table entry
vector_undef:
    b   exception_entry_undef

; Exception entry stub (saves context, calls handler)
exception_entry_undef:
    ; Save context
    ; ...
    bl  undef_handler
    ; Restore context
    ; ...
    movs pc, lr
```

```c
// C handler
void undef_handler(void) {
    // Handle undefined instruction
}
```

---

## 8. Exception Return Contract

### 8.1. Return Address Adjustment

Different exceptions require different LR adjustments:

| Exception       | Return Instruction | LR Adjustment |
|-----------------|-------------------|---------------|
| Undefined       | `movs pc, lr`     | LR = PC + 4 (no adjust) |
| SVC             | `movs pc, lr`     | LR = PC + 4 (no adjust) |
| Prefetch Abort  | `subs pc, lr, #4` | LR = PC + 4 (need -4) |
| Data Abort      | `subs pc, lr, #8` | LR = PC + 8 (need -8) |
| IRQ             | `subs pc, lr, #4` | LR = PC + 4 (need -4) |
| FIQ             | `subs pc, lr, #4` | LR = PC + 4 (need -4) |

**Critical:** Using wrong return instruction will cause kernel to jump to wrong address.

### 8.2. CPSR Restoration

Exception return **must** restore CPSR from SPSR:
- `movs pc, lr` - copies SPSR to CPSR automatically
- `subs pc, lr, #N` - also copies SPSR to CPSR

**Contract:** Exception return **must** use `s` suffix (SPSR restore) instructions.

---

## 9. Vector Table Placement in Memory

### 9.1. Linker Script Requirements

Vector table **must** be placed at a 32-byte aligned address within kernel image.

**Why 32-byte alignment:**
- ARM VBAR register ignores lower 5 bits (bits [4:0])
- This enforces natural 32-byte (2^5) alignment
- Vector table is 8 entries × 4 bytes = 32 bytes total

**Linker script section:**
```ld
SECTIONS {
    /* Kernel loaded at 0x80000000 by U-Boot */
    . = 0x80000000;
    
    .vectors : ALIGN(32) {
        __vectors_start = .;
        KEEP(*(.vectors))
        __vectors_end = .;
    }
    
    /* Rest of kernel sections */
}
```

**VBAR Configuration:**
VBAR register must be set to `__vectors_start` during boot (`entry.S`):
```asm
ldr r0, =__vectors_start
mcr p15, 0, r0, c12, c0, 0  ; Write VBAR (CP15 c12 c0 0)
isb                          ; Instruction Sync Barrier
```

**Verification:**
```bash
# Check __vectors_start alignment
arm-none-eabi-nm kernel.elf | grep __vectors_start
# Address must end in 0x00 or 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0
# Examples: 0x80000000 ✓, 0x80000020 ✓, 0x80000010 ✗
```

### 9.2. Assembly Vector Table Declaration

```asm
.section .vectors, "ax"
.align 5                       ; 32-byte alignment (2^5 = 32)
.global vectors
vectors:
    ldr pc, =reset_handler     ; +0x00: Reset (consistent style)
    ldr pc, =vector_undef      ; +0x04: Undefined
    ldr pc, =vector_svc        ; +0x08: SVC
    ldr pc, =vector_pabt       ; +0x0C: Prefetch Abort
    ldr pc, =vector_dabt       ; +0x10: Data Abort
    nop                        ; +0x14: Reserved
    ldr pc, =vector_irq        ; +0x18: IRQ
    ldr pc, =vector_fiq        ; +0x1C: FIQ
```

**Design choice: `ldr pc, =label` vs `b label`**
- `ldr pc, =label`: Full 32-bit address range, recommended for vector table
- `b label`: Relative branch, limited to ±32MB range
- Using `ldr pc` ensures vectors can jump anywhere in memory

**Lưu ý:**
- Reset handler (offset 0x00) thường không được sử dụng sau khi VBAR được set
- Trên AM335x, reset luôn bắt đầu từ Boot ROM (`0x4030CE00`)
- Keeping reset entry for completeness and consistency

### 9.3. Memory Region Protection

In phased approach:
- **Current phase:** No memory protection (MMU off).
- **Future phase:** Vector table region should be **read-only** after initialization.

---

## 10. Exception Stack Setup Contract

### 10.1. Stack Initialization Responsibility

**Boot code** (`entry.S`) **must** initialize all exception mode stacks:

```asm
; Setup UND mode stack
msr CPSR_c, #(MODE_UND | I_BIT | F_BIT)
ldr sp, =__und_stack_end

; Setup ABT mode stack
msr CPSR_c, #(MODE_ABT | I_BIT | F_BIT)
ldr sp, =__abt_stack_end

; Setup IRQ mode stack
msr CPSR_c, #(MODE_IRQ | I_BIT | F_BIT)
ldr sp, =__irq_stack_end

; Setup FIQ mode stack
msr CPSR_c, #(MODE_FIQ | I_BIT | F_BIT)
ldr sp, =__fiq_stack_end

; Back to SVC mode
msr CPSR_c, #(MODE_SVC | I_BIT | F_BIT)
ldr sp, =__svc_stack_end
```

### 10.2. Stack Overflow Detection

**Current phase:** No stack overflow detection.

**Future consideration:**
- Guard pages (requires MMU).
- Software stack canary.
- Dedicated overflow exception handler.

---

## 11. Contract Verification

### 11.1. Static Verification

Vector table contract có thể được xác minh thông qua:
- **Linker map:** Kiểm tra vector table được align 32-byte
- **Symbol check:** Kiểm tra symbol `__vectors_start` tồn tại
- **Objdump:** Kiểm tra vector table instructions
- **Size check:** Kiểm tra vector table có đúng 32 bytes

**Ví dụ verification:**
```bash
# Kiểm tra alignment
arm-none-eabi-nm kernel.elf | grep __vectors_start
# Địa chỉ phải kết thúc bằng 0x00 hoặc 0x20 (32-byte aligned)

# Kiểm tra kích thước vector table
arm-none-eabi-objdump -h kernel.elf | grep .vectors
# Phải hiển thị size = 0x20 (32 bytes)
```

### 11.2. Runtime Verification

Exception handling có thể được test bằng cách:
- **Deliberate undefined instruction:** `udf #0` hoặc `.word 0xe7f000f0`
- **Deliberate data abort:** Truy cập địa chỉ không hợp lệ
- **Deliberate prefetch abort:** Jump đến địa chỉ không hợp lệ

**Test harness nên:**
- Trigger exception một cách có chủ đích
- Verify handler được gọi
- Verify diagnostic output
- Verify VBAR được set đúng:
  ```c
  uint32_t read_vbar(void) {
      uint32_t vbar;
      asm volatile("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));
      return vbar;
  }
  ```

---

## 12. Ranh giới trách nhiệm

### 12.1. Trách nhiệm của Vector Table

- **Chỉ routing:** Jump đến exception entry code.
- **Không có logic:** Không save context, không xử lý handler logic.

### 12.2. Trách nhiệm của Exception Entry

- Save/restore context.
- Gọi C handler.
- Exception return với LR adjustment phù hợp.

### 12.3. Trách nhiệm của Kernel Handler

- Xử lý exception.
- Tạo diagnostics.
- Quyết định recovery hoặc halt.

### 12.4. Trách nhiệm của Boot Code

- Cấu hình VBAR trỏ đến kernel vector table (ví dụ: `__vectors_start`).
- Khởi tạo tất cả exception mode stacks.
- Setup SVC mode làm default execution mode.

**Cấu hình VBAR trong entry.S:**
```asm
; Sau khi setup stack, trước khi jump vào kernel_main
ldr r0, =__vectors_start
mcr p15, 0, r0, c12, c0, 0    ; Write to VBAR (CP15 c12)
isb                            ; Instruction Sync Barrier
```

**Critical:** Vector table **không được** đặt tại 0x00000000. Trên AM335x:
- 0x00000000 map tới GPMC/External Memory
- Write vào 0x00000000 sẽ gây data abort hoặc system crash
- VBAR cho phép đặt vector table linh hoạt trong kernel image

---

## 13. Explicit Non-Goals

Vector table contract **không định nghĩa** các nội dung sau:

- Exception handler logic.
- Context save/restore implementation.
- Exception priority handling.
- Nested exception behavior.
- Interrupt controller programming.
- Debug exception handling.

---

## 14. Quan hệ với các tài liệu khác

### Vector table contract này:
- Triển khai: `04_kernel/03_exception_model.md`
- Được sử dụng bởi: Exception entry assembly code
- Phụ thuộc vào: `02_boot/boot_contract.md` (CPU state)

### Vector table contract không thay thế:
- Boot documentation (reset vector handling)
- Interrupt model documentation (IRQ/FIQ logic)

Mọi thay đổi làm ảnh hưởng đến **vector table layout hoặc entry requirements** phải được phản ánh đồng bộ trong implementation code.

---

## 15. Ghi chú thiết kế

- Vector table contract **phải ổn định** - thay đổi nó sẽ ảnh hưởng toàn bộ exception handling.
- Entry point symbols **phải nhất quán** giữa assembly và C code.
- Stack sizes **phải được verify** để tránh overflow.
- LR adjustment **phải chính xác** cho từng exception type - sai adjustment = kernel corruption.

---

## 16. Implementation Checklist

Khi implement vector table, verify:

- [ ] Vector table section (.vectors) defined in linker script
- [ ] Vector table is 32-byte aligned (check __vectors_start address)
- [ ] Vector table is 32 bytes (8 entries × 4 bytes)
- [ ] All entry points defined (vector_undef, vector_svc, etc.)
- [ ] VBAR register set to __vectors_start in entry.S
- [ ] VBAR value verified at runtime (read CP15 c12)
- [ ] All exception mode stacks initialized
- [ ] Stack sizes meet minimum requirements
- [ ] LR adjustment correct for each exception type
- [ ] SPSR restored on exception return
- [ ] Test with deliberate exceptions