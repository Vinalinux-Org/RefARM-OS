/*
 * Software Division and Modulo for ARMv7-A
 * Implements __aeabi_idiv and __aeabi_idivmod
 */

.syntax unified
.arch armv7-a

.section .text

/* ============================================================
 * int __aeabi_idiv(int numerator, int denominator)
 * Signed integer division
 * Args: r0 = numerator, r1 = denominator
 * Returns: r0 = quotient
 * ============================================================ */
.global __aeabi_idiv
.type __aeabi_idiv, %function
__aeabi_idiv:
    push {r4, r5, lr}
    
    /* Handle division by zero */
    cmp r1, #0
    moveq r0, #0
    beq .idiv_end
    
    /* Save signs */
    eor r4, r0, r1      /* r4 = sign of result */
    
    /* Make operands positive */
    cmp r0, #0
    rsblt r0, r0, #0    /* r0 = abs(r0) */
    
    cmp r1, #0
    rsblt r1, r1, #0    /* r1 = abs(r1) */
    
    /* Perform unsigned division */
    mov r2, #0          /* quotient */
    mov r3, #1          /* bit position */
    
    /* Align divisor */
.idiv_align:
    cmp r1, r0
    movls r1, r1, lsl #1
    movls r3, r3, lsl #1
    bls .idiv_align
    
    /* Divide */
.idiv_loop:
    cmp r0, r1
    subge r0, r0, r1
    addge r2, r2, r3
    mov r1, r1, lsr #1
    movs r3, r3, lsr #1
    bne .idiv_loop
    
    /* Apply sign */
    mov r0, r2
    cmp r4, #0
    rsblt r0, r0, #0
    
.idiv_end:
    pop {r4, r5, pc}


/* ============================================================
 * void __aeabi_idivmod(int numerator, int denominator)
 * Signed integer division with modulo
 * Args: r0 = numerator, r1 = denominator
 * Returns: r0 = quotient, r1 = remainder
 * ============================================================ */
.global __aeabi_idivmod
.type __aeabi_idivmod, %function
__aeabi_idivmod:
    push {r4, r5, r6, lr}
    
    /* Save numerator and denominator */
    mov r4, r0
    mov r5, r1
    
    /* Call __aeabi_idiv to get quotient */
    bl __aeabi_idiv
    
    /* Calculate remainder: r1 = r4 - (r0 * r5) */
    mul r6, r0, r5
    sub r1, r4, r6
    
    pop {r4, r5, r6, pc}
