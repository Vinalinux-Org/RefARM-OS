/* ============================================================
 * svc_handler.c
 * ------------------------------------------------------------
 * Supervisor Call (SVC) Exception Handler
 * Implements the system call dispatcher and user/kernel boundary
 * ============================================================ */

#include "types.h"
#include "assert.h"
#include "trace.h"
#include "scheduler.h"
#include "uart.h"
#include "syscalls.h"
#include "mmu.h"
#include "vfs.h"
#include "string.h"

#define ELF_MAGIC_0 0x7F
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define ET_EXEC 2
#define EM_ARM 40
#define EV_CURRENT 1
#define PT_LOAD 1

#define EXEC_READ_CHUNK 128

extern uint8_t _shell_payload_start;
extern uint8_t _shell_payload_end;

typedef struct
{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_ehdr_t;

typedef struct
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

/*
 * SVC Context Structure (AAPCS Aligned)
 * Matches the stack layout pushed by exception_entry_svc
 */
struct svc_context
{
    uint32_t spsr; /* Saved Program Status Register */
    uint32_t pad;  /* Padding for 8-byte alignment */
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
    uint32_t lr; /* LR_svc - Points to instruction AFTER svc */
};

/*
 * Validation boundaries
 * Used to ensure user pointers are within allowed True User Space regions.
 */

/* ============================================================
 * Helper: Validate User Pointer
 * ============================================================
 * Enforces strict memory rules:
 * Pointers must point to the User Space region (0x40000000 -> +1MB).
 * This sandboxes User interactions preventing Kernel corruption.
 */
static int validate_user_pointer(const void *ptr, uint32_t len)
{
    uint32_t start = (uint32_t)ptr;
    uint32_t end = start + len;

    /* Check for overflow */
    if (end < start)
    {
        return E_PTR;
    }

    uint32_t allowed_start = USER_SPACE_VA;
    uint32_t allowed_end = USER_SPACE_VA + (USER_SPACE_MB * 1024 * 1024);

    /* Check bounds [start, end) within [allowed_start, allowed_end) */
    if (start >= allowed_start && end <= allowed_end)
    {
        return E_OK;
    }

    return E_PTR;
}

static int vfs_read_exact(int fd, void *buf, uint32_t len)
{
    uint8_t *dst = (uint8_t *)buf;
    uint32_t total = 0;

    while (total < len)
    {
        int n = vfs_read(fd, dst + total, len - total);
        if (n <= 0)
        {
            return E_FAIL;
        }
        total += (uint32_t)n;
    }

    return E_OK;
}

static int vfs_skip_bytes(int fd, uint32_t len)
{
    uint8_t tmp[EXEC_READ_CHUNK];
    uint32_t left = len;

    while (left > 0)
    {
        uint32_t chunk = (left > EXEC_READ_CHUNK) ? EXEC_READ_CHUNK : left;
        int n = vfs_read(fd, tmp, chunk);
        if (n <= 0)
        {
            return E_FAIL;
        }
        left -= (uint32_t)n;
    }

    return E_OK;
}

static int validate_elf_header(const elf32_ehdr_t *hdr)
{
    if (hdr->e_ident[0] != ELF_MAGIC_0 ||
        hdr->e_ident[1] != ELF_MAGIC_1 ||
        hdr->e_ident[2] != ELF_MAGIC_2 ||
        hdr->e_ident[3] != ELF_MAGIC_3)
    {
        return E_ARG;
    }

    if (hdr->e_ident[4] != ELFCLASS32 || hdr->e_ident[5] != ELFDATA2LSB)
    {
        return E_ARG;
    }

    if (hdr->e_type != ET_EXEC || hdr->e_machine != EM_ARM || hdr->e_version != EV_CURRENT)
    {
        return E_ARG;
    }

    if (hdr->e_phentsize != sizeof(elf32_phdr_t) || hdr->e_phnum == 0)
    {
        return E_ARG;
    }

    if (hdr->e_phoff != sizeof(elf32_ehdr_t))
    {
        return E_ARG;
    }

    if (validate_user_pointer((void *)hdr->e_entry, 4) != E_OK)
    {
        return E_PTR;
    }

    return E_OK;
}

/* ============================================================
 * Syscall Handlers
 * ============================================================ */

/* sys_write(const void *buf, uint32_t len) */
static int32_t sys_write(struct svc_context *ctx)
{
    const void *buf = NULL;
    uint32_t len = 0;
    int fd = 1;

    /* ABI compatibility:
     * - Legacy userspace:   write(buf, len)        in r0, r1
     * - Compiler runtime:   write(fd, buf, count)  in r0, r1, r2 */
    if (validate_user_pointer((const void *)ctx->r0, (uint32_t)ctx->r1) == E_OK)
    {
        buf = (const void *)ctx->r0;
        len = (uint32_t)ctx->r1;
    }
    else if (validate_user_pointer((const void *)ctx->r1, (uint32_t)ctx->r2) == E_OK)
    {
        fd = (int)ctx->r0;
        buf = (const void *)ctx->r1;
        len = (uint32_t)ctx->r2;
    }
    else
    {
        uart_printf("[SVC] Security Violation: Invalid Ptr 0x%08x\n", (uint32_t)ctx->r0);
        return E_PTR;
    }

    /* Only stdout/stderr are supported for now */
    if (fd != 1 && fd != 2)
    {
        return E_ARG;
    }

    /* 2. Logic (Direct Driver Call for now) */
    /* Only allow reasonable length to prevent DoS */
    if (len > 256)
    {
        return E_ARG;
    }

    /* We can safely cast because we validated range */
    const char *str = (const char *)buf;
    for (uint32_t i = 0; i < len; i++)
    {
        if (str[i] == '\n')
        {
            uart_putc('\r');
        }
        uart_putc(str[i]);
    }

    return (int32_t)len;
}

/* sys_exit(int status) */
static int32_t sys_exit(struct svc_context *ctx)
{
    int32_t status = (int32_t)ctx->r0;
    struct task_struct *current = scheduler_current_task();

    uart_printf("[SVC] Task %d exiting with status %d\n", current->id, status);

    /* Keep interactive system alive:
     * shell task is reused as an app container (exec). If that app exits,
     * restore original shell payload and jump back to shell entrypoint. */
    if (current && strcmp(current->name, "User App (Shell)") == 0)
    {
        uint32_t payload_size = (uint32_t)&_shell_payload_end - (uint32_t)&_shell_payload_start;
        uint8_t *src = &_shell_payload_start;
        uint8_t *dst = (uint8_t *)USER_SPACE_VA;

        for (uint32_t i = 0; i < payload_size; i++)
        {
            dst[i] = src[i];
        }

        ctx->r0 = 0;
        ctx->r1 = 0;
        ctx->r2 = 0;
        ctx->r3 = 0;
        ctx->lr = USER_SPACE_VA;

        uart_printf("[SVC] Shell restarted\n");
        return E_OK;
    }

    /* Terminate specific task */
    scheduler_terminate_task(current->id);

    /* Scheduler should switch away, but if we return, it's an error flow */
    return 0;
}

/* sys_yield() */
static int32_t sys_yield(struct svc_context *ctx)
{
    /*
     * CRITICAL FIX: Set need_reschedule flag BEFORE calling scheduler_yield
     *
     * Without this, voluntary yields may be ignored if the flag was already
     * cleared by a previous context switch. This causes tasks to get stuck
     * in busy-wait loops instead of properly yielding CPU to other tasks.
     *
     * Example failure scenario without this fix:
     * 1. Shell calls sys_yield() (need_reschedule=false from previous clear)
     * 2. sys_yield() calls scheduler_yield()
     * 3. scheduler_yield() sees need_reschedule=false, returns immediately
     * 4. Shell stuck in loop, never switches to Idle
     */
    extern volatile bool need_reschedule;
    need_reschedule = true;

    /* Voluntary Yield */
    scheduler_yield();
    return E_OK;
}

/* sys_read(void *buf, uint32_t len) */
static int32_t sys_read(struct svc_context *ctx)
{
    void *buf = NULL;
    uint32_t len = 0;
    int fd = 0;

    /* ABI compatibility:
     * - Legacy userspace: read(buf, len)         in r0, r1
     * - Compiler runtime: read(fd, buf, count)   in r0, r1, r2 */
    if (validate_user_pointer((void *)ctx->r0, (uint32_t)ctx->r1) == E_OK)
    {
        buf = (void *)ctx->r0;
        len = (uint32_t)ctx->r1;
    }
    else if (validate_user_pointer((void *)ctx->r1, (uint32_t)ctx->r2) == E_OK)
    {
        fd = (int)ctx->r0;
        buf = (void *)ctx->r1;
        len = (uint32_t)ctx->r2;
    }
    else
    {
        return E_PTR;
    }

    /* Only stdin is supported for now */
    if (fd != 0)
    {
        return E_ARG;
    }

    /* 1. Validation */
    int val_result = validate_user_pointer(buf, len);
    if (val_result != E_OK)
    {
        uart_printf("[SYS_READ] Validation FAILED: buf=0x%08x, len=%u, err=%d\n",
                    (uint32_t)buf, len, val_result);
        return E_PTR;
    }

    if (len == 0)
        return 0;

    /* 2. Logic: Read 1 byte (NON-BLOCKING)
     * Only supporting len=1 for now (getc style) to keep it simple.
     *
     * CRITICAL DESIGN DECISION:
     * We implement NON-BLOCKING I/O instead of blocking in kernel.
     *
     * WHY NOT BLOCK IN KERNEL?
     * Calling scheduler_yield() from within an SVC handler is DANGEROUS:
     * - We're in exception context with SVC stack frame
     * - Context switch expects normal task stack frame
     * - Stack corruption and crashes result
     *
     * PROPER BLOCKING I/O requires:
     * - Task sleep/wake mechanism
     * - Wait queues
     * - IRQ-driven wakeup
     *
     * Current implementation uses NON-BLOCKING I/O:
     * - Return 0 if no data available
     * - User code must call sys_yield() and retry
     */
    char *c_buf = (char *)buf;
    int c;

    c = uart_getc();
    if (c == -1)
    {
        // uart_printf("[SYS_READ] No data, returning 0\n");
        return 0; /* No data available - user should yield and retry */
    }

    // uart_printf("[SYS_READ] Got char: 0x%02x ('%c')\n", c, (c >= 32 && c <= 126) ? c : '?');

    /* Store result */
    *c_buf = (char)c;

    return 1; /* Read 1 byte */
}

/* sys_get_tasks(process_info_t *buf, uint32_t max_count) */
static int32_t sys_get_tasks(struct svc_context *ctx)
{
    void *buf = (void *)ctx->r0;
    uint32_t max_count = (uint32_t)ctx->r1;

    // Validate buffer (size = max_count * sizeof(process_info_t))
    uint32_t size = max_count * sizeof(process_info_t);
    if (validate_user_pointer(buf, size) != E_OK)
    {
        return E_PTR;
    }

    return scheduler_get_tasks(buf, max_count);
}

/* sys_get_meminfo(mem_info_t *buf) */
extern uint8_t _text_start[];
extern uint8_t _text_end[];
extern uint8_t _data_start[];
extern uint8_t _data_end[];
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];
extern uint8_t _stack_start[];
extern uint8_t _svc_stack_top[];
extern uint8_t _kernel_end[];

static int32_t sys_get_meminfo(struct svc_context *ctx)
{
    mem_info_t *buf = (mem_info_t *)ctx->r0;

    if (validate_user_pointer(buf, sizeof(mem_info_t)) != E_OK)
    {
        return E_PTR;
    }

    buf->total = 128 * 1024 * 1024; /* 128 MB */

    /* Calculate sizes */
    buf->kernel_text = (uint32_t)_text_end - (uint32_t)_text_start;
    buf->kernel_data = (uint32_t)_data_end - (uint32_t)_data_start;
    buf->kernel_bss = (uint32_t)_bss_end - (uint32_t)_bss_start;
    buf->kernel_stack = (uint32_t)_svc_stack_top - (uint32_t)_stack_start;

    /* Free memory = Total - (Kernel End - 0x80000000) */
    /* Note: This assumes simple linear allocation */
    uint32_t kernel_end = (uint32_t)_kernel_end;
    buf->free = buf->total - (kernel_end - 0x80000000);

    return E_OK;
}

/* sys_open(const char *path, int flags) */
static int32_t sys_open(struct svc_context *ctx)
{
    const char *path = (const char *)ctx->r0;
    int flags = (int)ctx->r1;

    /* Validate path pointer (estimate max path length) */
    if (validate_user_pointer(path, MAX_PATH) != E_OK)
    {
        return E_PTR;
    }

    /* Call VFS layer */
    return vfs_open(path, flags);
}

/* sys_read_file(int fd, void *buf, uint32_t len) */
static int32_t sys_read_file(struct svc_context *ctx)
{
    int fd = (int)ctx->r0;
    void *buf = (void *)ctx->r1;
    uint32_t len = (uint32_t)ctx->r2;

    /* CRITICAL: Validate buffer + length range as requested */
    if (validate_user_pointer(buf, len) != E_OK)
    {
        return E_PTR;
    }

    /* Call VFS layer */
    return vfs_read(fd, buf, len);
}

/* sys_close(int fd) */
static int32_t sys_close(struct svc_context *ctx)
{
    int fd = (int)ctx->r0;

    /* Call VFS layer */
    return vfs_close(fd);
}

/* sys_listdir(const char *path, file_info_t *entries, uint32_t max_entries) */
static int32_t sys_listdir(struct svc_context *ctx)
{
    const char *path = (const char *)ctx->r0;
    file_info_t *entries = (file_info_t *)ctx->r1;
    uint32_t max_entries = (uint32_t)ctx->r2;

    /* Validate path pointer */
    if (validate_user_pointer(path, MAX_PATH) != E_OK)
    {
        return E_PTR;
    }

    /* Validate entries buffer */
    uint32_t entries_size = max_entries * sizeof(file_info_t);
    if (validate_user_pointer(entries, entries_size) != E_OK)
    {
        return E_PTR;
    }

    /* Call VFS layer */
    return vfs_listdir(path, entries, max_entries);
}

/* sys_exec(const char *path) */
static int32_t sys_exec(struct svc_context *ctx)
{
    const char *path = (const char *)ctx->r0;
    elf32_ehdr_t ehdr;
    elf32_phdr_t phdr;
    int fd;

    if (validate_user_pointer(path, MAX_PATH) != E_OK)
    {
        return E_PTR;
    }

    fd = vfs_open(path, O_RDONLY);
    if (fd < 0)
    {
        return fd;
    }

    if (vfs_read_exact(fd, &ehdr, sizeof(ehdr)) != E_OK)
    {
        vfs_close(fd);
        return E_FAIL;
    }

    int hdr_ok = validate_elf_header(&ehdr);
    if (hdr_ok != E_OK)
    {
        vfs_close(fd);
        return hdr_ok;
    }

    for (uint32_t i = 0; i < ehdr.e_phnum; i++)
    {
        if (vfs_read_exact(fd, &phdr, sizeof(phdr)) != E_OK)
        {
            vfs_close(fd);
            return E_FAIL;
        }

        if (phdr.p_type != PT_LOAD)
        {
            continue;
        }

        if (phdr.p_filesz > phdr.p_memsz)
        {
            vfs_close(fd);
            return E_ARG;
        }

        if (validate_user_pointer((void *)phdr.p_vaddr, phdr.p_memsz) != E_OK)
        {
            vfs_close(fd);
            return E_PTR;
        }

        int seg_fd = vfs_open(path, O_RDONLY);
        if (seg_fd < 0)
        {
            vfs_close(fd);
            return seg_fd;
        }

        if (vfs_skip_bytes(seg_fd, phdr.p_offset) != E_OK)
        {
            vfs_close(seg_fd);
            vfs_close(fd);
            return E_FAIL;
        }

        if (vfs_read_exact(seg_fd, (void *)phdr.p_vaddr, phdr.p_filesz) != E_OK)
        {
            vfs_close(seg_fd);
            vfs_close(fd);
            return E_FAIL;
        }

        vfs_close(seg_fd);

        if (phdr.p_memsz > phdr.p_filesz)
        {
            memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
        }
    }

    vfs_close(fd);

    ctx->r0 = 0;
    ctx->r1 = 0;
    ctx->r2 = 0;
    ctx->r3 = 0;
    ctx->lr = ehdr.e_entry;

    return E_OK;
}

/* ============================================================
 * SVC Handler (Dispatcher)
 * ============================================================ */
void svc_handler(struct svc_context *ctx)
{
    /*
     * ABI:
     * R7 = Syscall Number
     * R0-R3 = Arguments
     * Return value -> R0
     */
    uint32_t syscall_num = ctx->r7;
    int32_t result = E_INVAL;

    static uint32_t svc_call_count = 0;
    svc_call_count++;

    /* DEBUG: Print every 5000 SVC calls */
    // if (svc_call_count % 5000 == 0) {
    //     uart_printf("[SVC] Call #%u: syscall=%u (YIELD=%u, READ=%u)\n",
    //                 svc_call_count, syscall_num, SYS_YIELD, SYS_READ);
    // }

    // TRACE_SCHED("SVC Entry: ID=%d, Args=0x%x, 0x%x", syscall_num, ctx->r0, ctx->r1);

    switch (syscall_num)
    {
    case SYS_WRITE:
        result = sys_write(ctx);
        break;

    case SYS_EXIT:
        result = sys_exit(ctx);
        break;

    case SYS_YIELD:
        result = sys_yield(ctx);
        break;

    case SYS_READ:
        result = sys_read(ctx);
        break;

    case SYS_GET_TASKS:
        result = sys_get_tasks(ctx);
        break;

    case SYS_GET_MEMINFO:
        result = sys_get_meminfo(ctx);
        break;

    case SYS_OPEN:
        result = sys_open(ctx);
        break;

    case SYS_READ_FILE:
        result = sys_read_file(ctx);
        break;

    case SYS_CLOSE:
        result = sys_close(ctx);
        break;

    case SYS_LISTDIR:
        result = sys_listdir(ctx);
        break;

    case SYS_EXEC:
        result = sys_exec(ctx);
        break;

    default:
        uart_printf("[SVC] ERROR: Unknown Syscall %d\n", syscall_num);
        result = E_INVAL;
        break;
    }

    /* Write return value back to User Context R0 */
    ctx->r0 = result;
}