#include "gdt.h"
#include "tss.h"
#include "../vga.h"
#include <stdint.h>

// GDT with 7 entries: null, kernel code, kernel data, user code, user data, TSS (2 slots)
gdt_entry_t gdt_entries[GDT_ENTRIES_COUNT];
gdt_ptr_t gdt_ptr;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // В 64-битном режиме base и limit игнорируются для code/data сегментов
    (void)base;   // Подавляем предупреждение о неиспользуемом параметре
    (void)limit;  // Подавляем предупреждение о неиспользуемом параметре
    
    gdt_entries[num].base_low = 0;
    gdt_entries[num].base_middle = 0; 
    gdt_entries[num].base_high = 0;
    
    gdt_entries[num].limit_low = 0xFFFF;
    gdt_entries[num].granularity = gran;
    gdt_entries[num].access = access;
}

void init_gdt(void) {
    puts("Setting up GDT for 64-bit long mode...\n");
    
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES_COUNT) - 1;
    gdt_ptr.base = (uint64_t)&gdt_entries;
    
    // Null segment (0x00)
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Kernel code segment (0x08) - 64-bit, Ring 0
    // Access: Present(1) + DPL(00) + System(1) + Executable(1) + Conforming(0) + Readable(1) + Accessed(0) = 10011010 = 0x9A
    // Granularity: Granular(1) + Size(0 for 64-bit) + Long(1) + Available(0) + Limit(1111) = 10101111 = 0xAF
    gdt_set_gate(1, 0, 0, 0x9A, 0xAF);
    
    // Kernel data segment (0x10) - Ring 0
    // Access: Present(1) + DPL(00) + System(1) + Executable(0) + Direction(0) + Writable(1) + Accessed(0) = 10010010 = 0x92
    // Granularity: Granular(1) + Size(1) + Long(0) + Available(0) + Limit(1111) = 11001111 = 0xCF
    gdt_set_gate(2, 0, 0, 0x92, 0xCF);
    
    // User code segment (0x18) - 64-bit, Ring 3
    // Access: Present(1) + DPL(11) + System(1) + Executable(1) + Conforming(0) + Readable(1) + Accessed(0) = 11111010 = 0xFA
    // Granularity: Same as kernel code for 64-bit
    gdt_set_gate(3, 0, 0, 0xFA, 0xAF);
    
    // User data segment (0x20) - Ring 3  
    // Access: Present(1) + DPL(11) + System(1) + Executable(0) + Direction(0) + Writable(1) + Accessed(0) = 11110010 = 0xF2
    // Granularity: Same as kernel data
    gdt_set_gate(4, 0, 0, 0xF2, 0xCF);
    
    // Load the GDT
    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr));
    
    // Инициализируем и загружаем TSS
    init_tss();

    // Перезагружаем сегментные регистры
    reload_segments();

    puts("GDT loaded for 64-bit mode\n");
}

// Перезагружает сегментные регистры после обновления GDT
void reload_segments(void) {
    __asm__ volatile (
        "pushq %[code_seg];"
        "leaq 9f(%%rip), %%rax;"
        "pushq %%rax;"
        "lretq;"
        "9:"
        "movw %[data_seg], %%ax;"
        "movw %%ax, %%ds; movw %%ax, %%es; movw %%ax, %%fs; movw %%ax, %%gs; movw %%ax, %%ss;"
        : : [code_seg] "i"(KERNEL_CODE_SEGMENT), [data_seg] "i"(KERNEL_DATA_SEGMENT) : "rax", "memory");
}

// Function to switch from kernel mode to user mode
void switch_to_user_mode(void) {
    puts("Attempting to switch to user mode (64-bit)...\n");
    
    // В 64-битном режиме переключение в user mode требует специальной настройки
    // Пока что просто проверим, что GDT загружена корректно
    
    puts("Testing GDT segments...\n");
    
    // Проверяем, что мы можем получить доступ к сегментам
    uint16_t cs, ds;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    
    puts("Current CS: ");
    puts_uint64_fg(cs, VGA_COLOR_WHITE);
    puts(", DS: ");
    puts_uint64_fg(ds, VGA_COLOR_WHITE);
    puts("\n");
    
    puts("GDT test completed. User mode switching requires syscall infrastructure.\n");
    puts("Use 'user-create' command to create user space processes instead.\n");
}
