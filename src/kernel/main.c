/* ============================================================
 * main_debug.c - UART RX INTERRUPT DEBUG VERSION
 * ------------------------------------------------------------
 * Purpose: Debug why UART RX interrupt is not firing
 * ============================================================
 */

#include <stddef.h>
#include "uart.h"
#include "watchdog.h"
#include "irq.h"
#include "intc.h"
#include "cpu.h"
#include "mmio.h"

void kernel_main(void)
{
    /* Early init */
    watchdog_disable();
    uart_init();
    
    uart_printf("\n\n");
    uart_printf("========================================\n");
    uart_printf("  UART RX INTERRUPT DEBUG SESSION\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("Target: BeagleBone Black (AM335x)\n");
    uart_printf("Goal: Debug why UART RX interrupt is not firing\n");
    uart_printf("\n");
    
    /* Initialize interrupt subsystem */
    uart_printf(">>> Phase 1: Initialize interrupt subsystem <<<\n\n");
    
    intc_init();
    irq_init();
    
    /* Configure UART RX interrupt */
    uart_printf(">>> Phase 2: Configure UART RX interrupt <<<\n\n");
    uart_enable_rx_interrupt();
    
    /* Check CPSR state before enabling */
    uart_printf(">>> Phase 3: Check CPSR state <<<\n\n");
    
    uint32_t cpsr;
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    uart_printf("CPSR (before enable) = 0x%08x\n", cpsr);
    uart_printf("  I bit (bit 7) = %u (1=disabled, 0=enabled)\n", (cpsr >> 7) & 1);
    uart_printf("  F bit (bit 6) = %u (1=disabled, 0=enabled)\n", (cpsr >> 6) & 1);
    uart_printf("  Mode (bits 4:0) = 0x%02x (0x13=SVC)\n", cpsr & 0x1F);
    uart_printf("\n");
    
    /* Enable IRQ in CPSR */
    uart_printf(">>> Phase 4: Enable IRQ in CPSR <<<\n\n");
    uart_printf("Calling irq_enable()...\n");
    irq_enable();
    
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    uart_printf("CPSR (after enable) = 0x%08x\n", cpsr);
    uart_printf("  I bit (bit 7) = %u\n", (cpsr >> 7) & 1);
    if (cpsr & (1 << 7)) {
        uart_printf("  ERROR: I bit still SET (IRQ still disabled!)\n");
    } else {
        uart_printf("  SUCCESS: I bit CLEARED (IRQ enabled)\n");
    }
    uart_printf("\n");
    
    /* Ready for testing */
    uart_printf("========================================\n");
    uart_printf("  SYSTEM READY FOR TESTING\n");
    uart_printf("========================================\n");
    uart_printf("\n");
    uart_printf("Instructions:\n");
    uart_printf("1. Type a character on the keyboard\n");
    uart_printf("2. Watch for [UART_IRQ] or [IRQ_DISPATCH] messages\n");
    uart_printf("3. If no messages appear, interrupt is NOT firing\n");
    uart_printf("\n");
    uart_printf("Debug commands:\n");
    uart_printf("  's' - Show statistics\n");
    uart_printf("  'r' - Show UART registers\n");
    uart_printf("  'i' - Show INTC registers\n");
    uart_printf("  'c' - Show CPSR state\n");
    uart_printf("\n");
    uart_printf(">>> TYPE A CHARACTER NOW <<<\n");
    uart_printf("\n");
    
    /* Main loop */
    uint32_t loop_count = 0;
    uint32_t last_irq_count = 0;
    
    while (1) {
        /* Try to read from buffer */
        int c = uart_getc();
        
        if (c >= 0) {
            /* Character received! */
            uart_printf("\n[MAIN] Got character from buffer: ");
            uart_printf("0x%02x", (unsigned char)c);
            uart_printf(" ('");
            uart_putc((char)c);
            uart_printf("')\n");
            
            /* Handle debug commands */
            switch (c) {
            case 's':  /* Statistics */
                uart_printf("\n=== Statistics ===\n");
                uart_printf("UART IRQ fires:    %u\n", uart_get_irq_fire_count());
                uart_printf("IRQ 72 dispatches: %u\n", irq_get_count(72));
                uart_printf("RX buffer avail:   %d\n", uart_rx_available());
                uart_printf("==================\n\n");
                break;
                
            case 'r':  /* UART registers */
                {
                    #define UART0_BASE 0x44E09000
                    uart_printf("\n=== UART Registers ===\n");
                    
                    uint32_t lsr = mmio_read32(UART0_BASE + 0x14);
                    uint32_t ier = mmio_read32(UART0_BASE + 0x04);
                    uint32_t iir = mmio_read32(UART0_BASE + 0x08);
                    uint32_t scr = mmio_read32(UART0_BASE + 0x40);
                    uint32_t lcr = mmio_read32(UART0_BASE + 0x0C);
                    
                    uart_printf("LSR  = 0x%08x\n", lsr);
                    uart_printf("  DR (bit 0) = %u (1=data ready)\n", lsr & 1);
                    uart_printf("  THRE (bit 5) = %u (1=THR empty)\n", (lsr >> 5) & 1);
                    uart_printf("IER  = 0x%08x\n", ier);
                    uart_printf("  RHR_IT (bit 0) = %u\n", ier & 1);
                    uart_printf("IIR  = 0x%08x\n", iir);
                    uart_printf("  IT_PENDING (bit 0) = %u (0=pending, 1=none)\n", iir & 1);
                    uart_printf("SCR  = 0x%08x\n", scr);
                    uart_printf("  RX_TRIG_GRANU1 (bit 7) = %u\n", (scr >> 7) & 1);
                    uart_printf("LCR  = 0x%08x\n", lcr);
                    uart_printf("======================\n\n");
                }
                break;
                
            case 'i':  /* INTC registers */
                {
                    #define INTC_BASE 0x48200000
                    
                    uart_printf("\n=== INTC Registers ===\n");
                    
                    uint32_t mir2 = mmio_read32(INTC_BASE + 0xC4);
                    uint32_t pending2 = mmio_read32(INTC_BASE + 0xD8);
                    uint32_t ilr72 = mmio_read32(INTC_BASE + 0x100 + (72 * 4));
                    uint32_t sir_irq = mmio_read32(INTC_BASE + 0x40);
                    
                    uart_printf("MIR2         = 0x%08x\n", mir2);
                    uart_printf("  IRQ 72 (bit 8) = %u (1=masked, 0=unmasked)\n", (mir2 >> 8) & 1);
                    uart_printf("PENDING_IRQ2 = 0x%08x\n", pending2);
                    uart_printf("  IRQ 72 (bit 8) = %u (1=pending)\n", (pending2 >> 8) & 1);
                    uart_printf("ILR[72]      = 0x%08x\n", ilr72);
                    uart_printf("  Priority = %u\n", (ilr72 >> 2) & 0x3F);
                    uart_printf("  FIQ/nIRQ = %u (0=IRQ)\n", ilr72 & 1);
                    uart_printf("SIR_IRQ      = 0x%08x\n", sir_irq);
                    uart_printf("  ACTIVEIRQ = %u\n", sir_irq & 0x7F);
                    uart_printf("======================\n\n");
                }
                break;
                
            case 'c':  /* CPSR */
                {
                    uint32_t cpsr_val;
                    asm volatile("mrs %0, cpsr" : "=r"(cpsr_val));
                    uart_printf("\n=== CPSR State ===\n");
                    uart_printf("CPSR = 0x%08x\n", cpsr_val);
                    uart_printf("  I bit (bit 7) = %u (1=IRQ disabled)\n", (cpsr_val >> 7) & 1);
                    uart_printf("  F bit (bit 6) = %u (1=FIQ disabled)\n", (cpsr_val >> 6) & 1);
                    uart_printf("  Mode = 0x%02x ", cpsr_val & 0x1F);
                    switch (cpsr_val & 0x1F) {
                    case 0x10: uart_printf("(USR)\n"); break;
                    case 0x11: uart_printf("(FIQ)\n"); break;
                    case 0x12: uart_printf("(IRQ)\n"); break;
                    case 0x13: uart_printf("(SVC)\n"); break;
                    case 0x17: uart_printf("(ABT)\n"); break;
                    case 0x1B: uart_printf("(UND)\n"); break;
                    case 0x1F: uart_printf("(SYS)\n"); break;
                    default: uart_printf("(UNKNOWN)\n"); break;
                    }
                    uart_printf("==================\n\n");
                }
                break;
                
            default:
                /* Echo character */
                uart_printf("Echoing: '");
                uart_putc((char)c);
                uart_printf("'\n\n");
                break;
            }
        }
        
        /* Periodic heartbeat */
        if (++loop_count >= 5000000) {
            uint32_t irq_count = uart_get_irq_fire_count();
            
            if (irq_count != last_irq_count) {
                uart_printf("[HEARTBEAT] IRQ activity detected! count=%u\n", irq_count);
                last_irq_count = irq_count;
            } else {
                uart_printf("[HEARTBEAT] No IRQ activity (count still %u)\n", irq_count);
            }
            
            loop_count = 0;
        }
        
        /* Low power wait */
        wfi();
    }
}