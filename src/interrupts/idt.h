#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// 64-bit IDT entry structure
struct idt_entry {
    uint16_t base_lo;      // Lower 16 bits of handler address
    uint16_t sel;          // Kernel segment selector
    uint8_t  ist;          // Interrupt Stack Table offset (bits 0-2), reserved (bits 3-7)
    uint8_t  flags;        // Type and attributes
    uint16_t base_mid;     // Middle 16 bits of handler address
    uint32_t base_hi;      // Upper 32 bits of handler address
    uint32_t reserved;     // Reserved, must be zero
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;         // 64-bit base address
} __attribute__((packed));

void init_idt();

#endif
