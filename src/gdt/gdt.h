#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT segment selectors
#define KERNEL_CODE_SEGMENT 0x08
#define KERNEL_DATA_SEGMENT 0x10  
#define USER_CODE_SEGMENT   0x1B  // 0x18 | 3 (Ring 3)
#define USER_DATA_SEGMENT   0x23  // 0x20 | 3 (Ring 3)
#define TSS_SEGMENT         0x28  // Селектор для TSS

#define GDT_ENTRIES_COUNT 7 // Null, KC, KD, UC, UD, TSS (2 слота)

// GDT entry structure for 64-bit mode
typedef struct {
    uint16_t limit_low;      // Limit bits 0-15
    uint16_t base_low;       // Base bits 0-15
    uint8_t base_middle;     // Base bits 16-23
    uint8_t access;          // Access flags
    uint8_t granularity;     // Granularity and limit bits 16-19
    uint8_t base_high;       // Base bits 24-31
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure  
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

void init_gdt(void);
void switch_to_user_mode(void);
void reload_segments(void); // Функция для перезагрузки сегментов

#endif // GDT_H
