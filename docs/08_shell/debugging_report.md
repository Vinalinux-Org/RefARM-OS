# Shell Debugging Report

## 1. Context Switch Crash (Data Abort)

### Issue Description
The system crashed with a **Data Abort** exception immediately after the Shell task yielded back to the Idle task.
- **Symptoms**: `PC` corrupted (pointing to data), `CPSR` invalid.
- **Root Cause**: The `context_switch` function in `context_switch.S` was attempting to restore **Frame 1** (SPSR, R0-R12, PC) from the stack. However, **Frame 1** only exists on the initial stack setup for new tasks. For a running task that voluntary yields, only **Frame 2** (Callee-saved registers + LR) is saved.
- **Result**: The restore instruction popped garbage data into PC and CPSR.

### Fix
Removed the code block in `context_switch.S` that attempted to restore Frame 1. The context switch now simply returns to the caller (`bx lr`) after restoring callee-saved registers (Frame 2).

### Diagram: Context Switch Stack Handling

```mermaid
sequenceDiagram
    participant Shell Task
    participant Context Switch
    participant Idle Task
    
    Note over Shell Task: Running (User Mode)
    Shell Task->>Context Switch: SVC (Yield)
    
    Note over Context Switch: Save Frame 2 (R4-R11, LR)
    
    Context Switch->>Context Switch: **BUG**: Attempted to restore Frame 1 (PC, SPSR)\n(Frame 1 does not exist here!)
    Context Switch--xShell Task: CRASH (Data Abort)

    Note over Context Switch: **FIX**: Return via LR
    Context Switch->>Idle Task: Returns to caller (svc_handler)
    Note over Idle Task: Resumes Execution
```

---

## 2. Shell Input Failure & Lag

### Issue Description
1.  **Input Failure**: Coding in the shell did not work. `sys_read` always returned 0.
    - **Root Cause**: The UART RX interrupt was never enabled. The function `uart_enable_rx_interrupt()` existed but was not called in `kernel_main`.
    - **Fix**: Added `uart_enable_rx_interrupt()` call in `main.c`.
2.  **Input Lag**: Typing characters caused system unresponsive behavior and lost keys.
    - **Root Cause**: The `svc_handler` contained a debug `printf` inside the SVC dispatch loop. Since the shell calls `sys_read` (SVC #3) in a tight non-blocking loop, this caused thousands of print calls per second, saturating the UART output buffer and blocking the CPU.
    - **Fix**: Removed/Silenced debug logs in `svc_handler.c`, `uart.c`, and scheduler.

### Diagram: Corrected Input Flow

```mermaid
flowchart TD
    subgraph Hardware
    K[Keyboard Press] -->|UART RX| U[UART Hardware]
    U -->|IRQ 72| I[INTC Controller]
    end
    
    subgraph Kernel
    I -->|IRQ Entry| H[uart_rx_irq_handler]
    H -->|Store| B[(Ring Buffer)]
    end
    
    subgraph User Space
    S[Shell Task] -->|Loop| R[sys_read]
    R -->|SVC Call| KERN[Kernel Syscall]
    KERN -->|Check| B
    B -->|Return Char| S
    end
    
    style H fill:#f9f,stroke:#333
    style B fill:#bfb,stroke:#333
    style S fill:#ccf,stroke:#333
```
