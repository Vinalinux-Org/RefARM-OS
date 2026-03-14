# 03 - Memory Management and MMU

## Overview

VinixOS sử dụng ARMv7-A MMU để implement true 3G/1G virtual memory split:
- **User Space**: 0x40000000 - 0x40FFFFFF (16MB)
- **Kernel Space**: 0xC0000000 - 0xC7FFFFFF (128MB)

Document này giải thích cách MMU được configure và cách address translation hoạt động.

## ARMv7-A MMU Basics

### Translation Table

VinixOS dùng **1-level page table** với **section descriptors** (1MB granularity):

```
Page Global Directory (PGD):
- 4096 entries × 4 bytes = 16KB
- Must be 16KB aligned
- Each entry maps 1MB of VA space
- Entry format: [PA base][Flags]
```

**Tại sao 1-level**: Đơn giản, đủ cho embedded OS. 2-level (với page tables) phức tạp hơn và cần thêm memory.

**Section Descriptor Format** (32-bit):
```
[31:20] Section Base Address (PA[31:20])
[19:12] Reserved/Implementation Defined
[11:10] AP[1:0] - Access Permissions
[9]     Reserved
[8:5]   Domain
[4]     XN - Execute Never
[3:2]   C, B - Cacheable, Bufferable
[1:0]   Type (10 = Section)
```


## Memory Layout

### Physical Memory Map

```
0x00000000 - 0x001FFFFF: Boot ROM (2MB)
0x402F0000 - 0x4030FFFF: Internal SRAM (128KB)
0x44E00000 - 0x44E0FFFF: L4_WKUP Peripherals (UART0, CM_PER, WDT)
0x48000000 - 0x482FFFFF: L4_PER Peripherals (INTC, Timer, GPIO)
0x4C000000 - 0x4C000FFF: EMIF (DDR Controller)
0x80000000 - 0x9FFFFFFF: DDR3 RAM (512MB)
```

### Virtual Memory Map (Final State)

```
User Space:
0x40000000 - 0x40FFFFFF: User Applications (16MB)
                         → PA 0x80000000
                         Cached, User RW, Kernel RW

Kernel Space:
0xC0000000 - 0xC00FFFFF: Kernel Code + Data (1MB)
                         → PA 0x80000000
                         Cached, Kernel-only
0xC0100000 - 0xC7FFFFFF: Kernel Heap/Stack (127MB)
                         → PA 0x80100000
                         Cached, Kernel-only

Peripherals (Identity Mapped):
0x44E00000 - 0x44E0FFFF: L4_WKUP
                         Strongly Ordered, Kernel-only
0x48000000 - 0x482FFFFF: L4_PER
                         Strongly Ordered, Kernel-only
```

**True 3G/1G Split**: User và Kernel không share VA space. User có dedicated 1GB region (chỉ dùng 16MB), Kernel có dedicated 1GB region (dùng 128MB).


## MMU Configuration

File: `VinixOS/kernel/src/kernel/mmu/mmu.c`

### Section Descriptor Flags

```c
/* Kernel RAM: Cached, Kernel-only (AP=01) */
#define MMU_SECT_KERNEL_RAM  (0x00000002 | /* Section */ \
                              (0x1 << 10) | /* AP[1:0]=01 Kernel RW */ \
                              (0x0 << 5)  | /* Domain 0 */ \
                              (0x1 << 3)  | /* C=1 Cacheable */ \
                              (0x1 << 2))   /* B=1 Bufferable */

/* User RAM: Cached, User RW, Kernel RW (AP=11) */
#define MMU_SECT_USER_RAM    (0x00000002 | /* Section */ \
                              (0x3 << 10) | /* AP[1:0]=11 User RW */ \
                              (0x0 << 5)  | /* Domain 0 */ \
                              (0x1 << 3)  | /* C=1 Cacheable */ \
                              (0x1 << 2))   /* B=1 Bufferable */

/* Peripheral: Strongly Ordered, Kernel-only */
#define MMU_SECT_PERIPHERAL  (0x00000002 | /* Section */ \
                              (0x1 << 10) | /* AP[1:0]=01 Kernel RW */ \
                              (0x0 << 5)  | /* Domain 0 */ \
                              (0x0 << 3)  | /* C=0 Non-cacheable */ \
                              (0x0 << 2))   /* B=0 Non-bufferable */
```

**Access Permissions (AP)**:
- AP=01: Kernel RW, User No Access
- AP=11: Kernel RW, User RW

**Cache Policy**:
- RAM: C=1, B=1 (Write-back cached)
- Peripherals: C=0, B=0 (Strongly Ordered - no reordering, no caching)

**Domain**: Tất cả sections dùng Domain 0, set DACR = CLIENT mode (check AP bits).


### Page Table Build (Phase A)

Function: `mmu_build_page_table_boot()` - chạy tại PA trước khi enable MMU.

```c
void mmu_build_page_table_boot(uint32_t *pgd_pa) {
    /* Clear all entries */
    for (i = 0; i < MMU_L1_ENTRIES; i++) {
        pgd_pa[i] = 0;  /* Translation Fault */
    }
    
    /* Peripheral identity maps */
    for (i = 0; i < PERIPH_L4_WKUP_SECTIONS; i++) {
        pa = PERIPH_L4_WKUP_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }
    
    for (i = 0; i < PERIPH_L4_PER_SECTIONS; i++) {
        pa = PERIPH_L4_PER_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }
    
    /* User Space: VA 0x40000000 → PA 0x80000000 */
    for (i = 0; i < USER_SPACE_MB; i++) {
        pa = USER_SPACE_PA + (i * MMU_SECTION_SIZE);
        va_idx = (USER_SPACE_VA + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_USER_RAM;
    }
    
    /* Kernel DDR: VA 0xC0000000 → PA 0x80000000 */
    for (i = 0; i < KERNEL_DDR_MB; i++) {
        pa = KERNEL_DDR_PA + (i * MMU_SECTION_SIZE);
        va_idx = (KERNEL_DDR_VA + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_KERNEL_RAM;
    }
    
    /* Temporary identity map: VA 0x80000000 → PA 0x80000000 */
    for (i = 0; i < BOOT_IDENTITY_MB; i++) {
        pa = BOOT_IDENTITY_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_KERNEL_RAM;
    }
}
```

**Identity Mapping**: Temporary mapping để CPU có thể continue execute tại PA sau khi enable MMU. Sẽ bị remove bởi mmu_init().

**Index Calculation**: `va_idx = VA >> 20` (shift right 20 bits để lấy entry index trong PGD).


### MMU Enable Sequence

Function: `mmu_enable()` - assembly code trong `mmu.c`

```c
void mmu_enable(uint32_t *pgd_base) {
    /* 1. Set Translation Table Base Register 0 (TTBR0) */
    __asm__ volatile("mcr p15, 0, %0, c2, c0, 0" :: "r"(pgd_base));
    
    /* 2. Set Domain Access Control Register (DACR)
     *    Domain 0 = CLIENT (check AP bits) */
    uint32_t dacr = 0x00000001;  /* D0=01 (CLIENT) */
    __asm__ volatile("mcr p15, 0, %0, c3, c0, 0" :: "r"(dacr));
    
    /* 3. Enable MMU in System Control Register (SCTLR) */
    uint32_t sctlr;
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr |= 0x1;  /* M bit - Enable MMU */
    __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr));
    
    /* 4. Instruction Synchronization Barrier */
    __asm__ volatile("isb");
}
```

**CP15 Registers**:
- c2 (TTBR0): Page table base address
- c3 (DACR): Domain access control
- c1 (SCTLR): System control (MMU enable bit)

**ISB**: Ensure MMU enable takes effect before next instruction fetch.

### Remove Identity Mapping (Phase B)

Function: `mmu_init()` - gọi từ kernel_main() sau trampoline

```c
void mmu_init(void) {
    /* Remove identity mapping */
    for (i = 0; i < BOOT_IDENTITY_MB; i++) {
        pa = BOOT_IDENTITY_PA + (i * MMU_SECTION_SIZE);
        pgd[pa >> MMU_SECTION_SHIFT] = 0;  /* FAULT */
    }
    
    /* Flush TLB - clear cached translations */
    __asm__ volatile(
        "mov r0, #0\n\t"
        "mcr p15, 0, r0, c8, c7, 0\n\t"  /* TLBIALL - Invalidate entire TLB */
        "dsb\n\t"  /* Data Synchronization Barrier */
        "isb\n\t"  /* Instruction Synchronization Barrier */
    );
    
    /* Update VBAR to high VA */
    uint32_t vbar_va = (uint32_t)&_boot_start + VA_OFFSET;
    __asm__ volatile("mcr p15, 0, %0, c12, c0, 0" :: "r"(vbar_va));
}
```

**TLB Flush**: TLB cache translations. Sau khi modify page table, phải flush TLB để CPU re-translate.

**VBAR Update**: Exception vectors physical location không đổi, nhưng VBAR phải point đến VA.


## Address Translation Flow

### Translation Process

```mermaid
graph LR
    A[Virtual Address] --> B[VA[31:20] = Index]
    B --> C[PGD Entry = pgd[index]]
    C --> D{Entry Valid?}
    D -->|No| E[Translation Fault]
    D -->|Yes| F[PA[31:20] = Entry[31:20]]
    F --> G[PA[19:0] = VA[19:0]]
    G --> H[Physical Address]
```

**Example**: Translate VA 0xC0001234

1. Index = 0xC0001234 >> 20 = 0xC00 = 3072
2. PGD Entry = pgd[3072] = 0x80000C0E
   - PA base = 0x80000000
   - Flags = 0xC0E (Section, Cached, Kernel-only)
3. PA = 0x80000000 | (0xC0001234 & 0xFFFFF) = 0x80001234

### Permission Check

Khi CPU access memory, MMU check:

1. **Translation valid?** Entry != 0
2. **Domain access?** DACR[domain] == CLIENT → check AP
3. **Access permission?** 
   - Kernel mode: AP=01 or AP=11 → OK
   - User mode: AP=11 → OK, AP=01 → Permission Fault
4. **Execute permission?** XN bit (not used in VinixOS)

**Fault Types**:
- Translation Fault: Entry = 0 (unmapped VA)
- Permission Fault: AP check failed
- Domain Fault: DACR[domain] == NO_ACCESS


## Memory Regions Detail

### User Space (0x40000000 - 0x40FFFFFF)

```
0x40000000 - 0x400FFFFF: User Code + Data (1MB)
                         Shell binary loaded here
0x40100000 - 0x400FC000: User Stack (grows down, 16KB)
0x40100000 - 0x40FFFFFF: Reserved for future use
```

**Properties**:
- Mapped to PA 0x80000000 (same physical memory as kernel!)
- AP=11: User RW, Kernel RW
- Cached (C=1, B=1)

**Tại sao share physical memory**: Đơn giản cho reference OS. Production OS sẽ dùng separate physical pages.

### Kernel Space (0xC0000000 - 0xC7FFFFFF)

```
0xC0000000 - 0xC0000FFF: Exception Vectors (4KB)
0xC0001000 - 0xC00XXXXX: Kernel .text (code)
0xC00XXXXX - 0xC00YYYYY: Kernel .rodata (read-only data)
0xC00YYYYY - 0xC00ZZZZZ: Kernel .data (initialized data)
0xC00ZZZZZ - 0xC0100000: Kernel .bss (uninitialized data)
0xC0100000 - 0xC0110000: Kernel Stacks (64KB)
  - SVC stack: 16KB
  - IRQ stack: 16KB
  - ABT stack: 16KB
  - UND stack: 8KB
  - FIQ stack: 8KB
0xC0110000 - 0xC7FFFFFF: Kernel Heap (future use)
```

**Properties**:
- Mapped to PA 0x80000000+
- AP=01: Kernel-only
- Cached (C=1, B=1)

**Linker Script**: `kernel/linker/kernel.ld` defines exact layout.

### Peripheral Space

**L4_WKUP (0x44E00000 - 0x44E0FFFF)**:
- UART0: 0x44E09000
- Control Module: 0x44E10000
- CM_PER (Clock Manager): 0x44E00000
- WDT1 (Watchdog): 0x44E35000

**L4_PER (0x48000000 - 0x482FFFFF)**:
- INTC: 0x48200000
- DMTimer2: 0x48040000
- GPIO: 0x4804C000

**Properties**:
- Identity mapped (VA == PA)
- AP=01: Kernel-only
- Strongly Ordered (C=0, B=0)

**Tại sao Strongly Ordered**: Peripheral registers không được cache hoặc reorder. Mỗi access phải hit hardware.


## Key Design Decisions

### 1. True 3G/1G Split

**Decision**: User space tại 0x40000000, Kernel space tại 0xC0000000.

**Rationale**:
- Clear separation giữa user và kernel VA
- Catch bugs: Kernel dereference user pointer sẽ work (cùng PA), nhưng rõ ràng là cross-boundary access
- Standard Linux-like layout (dễ hiểu cho developers)

**Alternative**: Kernel tại 0x80000000 (identity map). Rejected vì blur boundary giữa PA và VA.

### 2. 1-Level Page Table

**Decision**: Chỉ dùng L1 section descriptors (1MB granularity).

**Rationale**:
- Đơn giản: Không cần L2 page tables
- Đủ cho embedded: 16MB user space, 128MB kernel space
- Performance: Ít TLB misses (mỗi entry map 1MB thay vì 4KB)

**Tradeoff**: Waste memory nếu chỉ dùng vài KB trong 1MB section. Acceptable cho reference OS.

### 3. Shared Physical Memory

**Decision**: User space và kernel space map đến cùng PA 0x80000000.

**Rationale**:
- Đơn giản: Không cần memory allocator
- Copy-free: Kernel có thể access user memory trực tiếp
- Đủ cho single-user OS

**Production OS**: Sẽ dùng separate physical pages cho mỗi process, implement copy-on-write, demand paging, etc.

### 4. Static Page Table

**Decision**: Page table được build 1 lần lúc boot, không modify runtime.

**Rationale**:
- Đơn giản: Không cần dynamic mapping
- Predictable: Memory layout fixed
- Đủ cho reference OS

**Production OS**: Sẽ dynamic map/unmap pages cho process creation, mmap, etc.

## Common Pitfalls

### 1. Forget TLB Flush

**Problem**: Modify page table nhưng không flush TLB → CPU dùng stale translation.

**Solution**: Luôn flush TLB sau khi modify page table:
```c
__asm__ volatile("mcr p15, 0, r0, c8, c7, 0" ::: "memory");  /* TLBIALL */
```

### 2. Wrong Cache Policy cho Peripherals

**Problem**: Cache peripheral registers → stale reads, lost writes.

**Solution**: Peripherals phải dùng Strongly Ordered (C=0, B=0).

### 3. Identity Mapping Leak

**Problem**: Quên remove identity mapping → code có thể accidentally dereference PA.

**Solution**: Remove identity mapping ngay sau trampoline trong mmu_init().

### 4. VBAR Not Updated

**Problem**: VBAR vẫn point đến PA sau enable MMU → exception vectors không resolve.

**Solution**: Update VBAR đến VA trong mmu_init().

## Key Takeaways

1. **MMU provides isolation**: User code không thể access kernel memory (AP bits enforce).

2. **Two-phase MMU setup**: Phase A (boot) enable với identity map, Phase B (runtime) remove identity map.

3. **Translation is transparent**: Sau enable MMU, tất cả memory access đều qua MMU. Code không cần aware.

4. **Cache policy matters**: RAM cached, peripherals strongly ordered.

5. **TLB is a cache**: Phải flush sau modify page table.

6. **1MB granularity is enough**: Cho embedded OS, section descriptors đơn giản và đủ dùng.
