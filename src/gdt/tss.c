#include "tss.h"
#include "../memory/vmm.h" // для kmalloc
#include "../std/string.h" // для memset
#include "gdt.h"           // для доступа к GDT

extern gdt_entry_t gdt_entries[GDT_ENTRIES_COUNT]; // Получаем доступ к GDT
extern gdt_ptr_t gdt_ptr;

static tss_t kernel_tss;

// Функция для установки дескриптора TSS в GDT
static void gdt_set_tss(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    tss_entry_t* tss_entry = (tss_entry_t*)&gdt_entries[num];
    
    tss_entry->base_low    = base & 0xFFFF;
    tss_entry->base_middle = (base >> 16) & 0xFF;
    tss_entry->base_high   = (base >> 24) & 0xFF;
    tss_entry->base_upper  = (base >> 32) & 0xFFFFFFFF;

    tss_entry->limit_low   = limit & 0xFFFF;
    tss_entry->limit_high_and_flags = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    
    tss_entry->access      = access;
    tss_entry->reserved    = 0;
}

void init_tss(void) {
    // Выделяем память для стека ядра, который будет использоваться при прерываниях из user-mode
    uint64_t kernel_stack = (uint64_t)kmalloc(4096);
    
    // Инициализируем TSS
    memset(&kernel_tss, 0, sizeof(tss_t));
    kernel_tss.rsp0 = kernel_stack + 4096; // Вершина стека
    
    // Добавляем TSS в GDT. TSS занимает два слота GDT.
    gdt_set_tss(5, (uint64_t)&kernel_tss, sizeof(tss_t) - 1, 0x89, 0x00);
    // Access: Present=1, DPL=0, Type=9 (64-bit TSS Available)

    // Загружаем TSS
    __asm__ volatile ("ltr %%ax" : : "a"(TSS_SEGMENT));
}

// Устанавливает вершину стека ядра для текущего CPU
void set_kernel_stack(uint64_t stack) {
    kernel_tss.rsp0 = stack;
}