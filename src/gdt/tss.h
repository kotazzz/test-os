#ifndef TSS_H
#define TSS_H

#include <stdint.h>

// Структура TSS для 64-битного режима
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;       // Указатель на стек Ring 0
    uint64_t rsp1;       // Указатель на стек Ring 1
    uint64_t rsp2;       // Указатель на стек Ring 2
    uint64_t reserved1;
    uint64_t ist[7];     // Interrupt Stack Table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset; // I/O Map Base Address
} __attribute__((packed)) tss_t;

// Структура дескриптора TSS в GDT
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  limit_high_and_flags;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) tss_entry_t;

void init_tss(void);
void set_kernel_stack(uint64_t stack);

#endif // TSS_H