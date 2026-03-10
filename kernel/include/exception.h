/* ============================================================
 * exception.h
 * ------------------------------------------------------------
 * Exception handling interface
 * Target: ARM Cortex-A8 (ARMv7-A)
 * ============================================================ */

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "types.h"

/* ============================================================
 * Exception Context Structure
 * ------------------------------------------------------------
 * Saved by exception entry stubs (assembly)
 * Passed to C exception handlers
 * ============================================================ */

/**
 * Exception context frame
 * 
 * Layout matches the order pushed by exception_entry stubs:
 * 1. r0-r12, LR (saved by STMFD first - CRITICAL!)
 * 2. SPSR (saved last)
 * 
 * Stack grows downward, so SPSR is at lowest address (SP points here)
 * 
 * Total size: 15 words (60 bytes)
 */
struct exception_context {
    uint32_t spsr;      /* Saved Program Status Register (at SP) */
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
    uint32_t lr;        /* Return address (adjusted by entry stub) */
};

/* ============================================================
 * Exception Handler Prototypes
 * ------------------------------------------------------------
 * Called from exception entry stubs
 * Must NOT return (fatal exceptions)
 * ============================================================ */

/**
 * Undefined Instruction Exception Handler
 * @param ctx Pointer to exception context on stack
 * 
 * Called when CPU encounters invalid/unsupported instruction
 * This is a FATAL exception - kernel will halt
 */
void handle_undefined_instruction(struct exception_context *ctx);

/**
 * Supervisor Call (SVC) Exception Handler
 * @param ctx Pointer to exception context on stack
 * 
 * Called when SVC instruction is executed
 * Not used in current phase (no syscalls)
 */
void handle_svc(struct exception_context *ctx);

/**
 * Prefetch Abort Exception Handler
 * @param ctx Pointer to exception context on stack
 * 
 * Called when instruction fetch fails (bad address, MMU fault, etc)
 * This is a FATAL exception - kernel will halt
 */
void handle_prefetch_abort(struct exception_context *ctx);

/**
 * Data Abort Exception Handler
 * @param ctx Pointer to exception context on stack
 * 
 * Called when data access fails (bad address, MMU fault, alignment, etc)
 * This is a FATAL exception - kernel will halt
 */
void handle_data_abort(struct exception_context *ctx);

/**
 * IRQ Exception Handler (stub)
 * @param ctx Pointer to exception context on stack
 * 
 * Called when hardware interrupt occurs
 * Not implemented in current phase (IRQ masked)
 */
void handle_irq(struct exception_context *ctx);

/**
 * FIQ Exception Handler (stub)
 * @param ctx Pointer to exception context on stack
 * 
 * Called when fast interrupt occurs
 * Not used in current phase (FIQ always masked)
 */
void handle_fiq(struct exception_context *ctx);

#endif /* EXCEPTION_H */