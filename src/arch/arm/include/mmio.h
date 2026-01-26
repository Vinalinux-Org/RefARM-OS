/* ============================================================
 * mmio.h
 * ------------------------------------------------------------
 * Memory-Mapped I/O primitives
 * Architecture: ARMv7-A
 * ============================================================
 */

#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

/* ============================================================
 * 32-bit MMIO operations
 * ============================================================
 */

/**
 * Write 32-bit value to memory-mapped register
 * @param addr Physical address
 * @param val  Value to write
 */
static inline void mmio_write32(uint32_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

/**
 * Read 32-bit value from memory-mapped register
 * @param addr Physical address
 * @return Value read
 */
static inline uint32_t mmio_read32(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

/* ============================================================
 * 16-bit MMIO operations (for peripherals that need it)
 * ============================================================
 */

static inline void mmio_write16(uint32_t addr, uint16_t val)
{
    *(volatile uint16_t *)addr = val;
}

static inline uint16_t mmio_read16(uint32_t addr)
{
    return *(volatile uint16_t *)addr;
}

/* ============================================================
 * 8-bit MMIO operations
 * ============================================================
 */

static inline void mmio_write8(uint32_t addr, uint8_t val)
{
    *(volatile uint8_t *)addr = val;
}

static inline uint8_t mmio_read8(uint32_t addr)
{
    return *(volatile uint8_t *)addr;
}

#endif /* MMIO_H */