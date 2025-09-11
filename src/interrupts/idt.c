#include "idt.h"
#include "isr.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

// Функция загрузки IDT
void lidt(struct idt_ptr* idtp) {
    __asm__ volatile ("lidt (%0)" : : "r"(idtp));
}

void set_idt_gate(uint8_t n, uint64_t handler, uint16_t sel, uint8_t flags) {
    idt[n].base_lo = handler & 0xFFFF;
    idt[n].sel = sel;
    idt[n].ist = 0;                                    // No IST used
    idt[n].flags = flags;
    idt[n].base_mid = (handler >> 16) & 0xFFFF;
    idt[n].base_hi = (handler >> 32) & 0xFFFFFFFF;
    idt[n].reserved = 0;
}

void init_idt() {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)&idt;

    // Устанавливаем специализированные обработчики для исключений (0-19)
    set_idt_gate(0, (uint64_t)isr0, 0x08, 0x8E);   // Division by Zero
    set_idt_gate(1, (uint64_t)isr1, 0x08, 0x8E);   // Debug
    set_idt_gate(2, (uint64_t)isr2, 0x08, 0x8E);   // NMI
    set_idt_gate(3, (uint64_t)isr3, 0x08, 0x8E);   // Breakpoint
    set_idt_gate(4, (uint64_t)isr4, 0x08, 0x8E);   // Overflow
    set_idt_gate(5, (uint64_t)isr5, 0x08, 0x8E);   // Bound Range Exceeded
    set_idt_gate(6, (uint64_t)isr6, 0x08, 0x8E);   // Invalid Opcode
    set_idt_gate(7, (uint64_t)isr7, 0x08, 0x8E);   // Device Not Available
    set_idt_gate(8, (uint64_t)isr8, 0x08, 0x8E);   // Double Fault
    set_idt_gate(9, (uint64_t)isr9, 0x08, 0x8E);   // Coprocessor Segment Overrun
    set_idt_gate(10, (uint64_t)isr10, 0x08, 0x8E); // Invalid TSS
    set_idt_gate(11, (uint64_t)isr11, 0x08, 0x8E); // Segment Not Present
    set_idt_gate(12, (uint64_t)isr12, 0x08, 0x8E); // Stack Segment Fault
    set_idt_gate(13, (uint64_t)isr13, 0x08, 0x8E); // General Protection Fault
    set_idt_gate(14, (uint64_t)isr14, 0x08, 0x8E); // Page Fault
    set_idt_gate(15, (uint64_t)isr15, 0x08, 0x8E); // Reserved
    set_idt_gate(16, (uint64_t)isr16, 0x08, 0x8E); // x87 FPU Exception
    set_idt_gate(17, (uint64_t)isr17, 0x08, 0x8E); // Alignment Check
    set_idt_gate(18, (uint64_t)isr18, 0x08, 0x8E); // Machine Check
    set_idt_gate(19, (uint64_t)isr19, 0x08, 0x8E); // SIMD Exception

    // Заполняем остальные дескрипторы дефолтным обработчиком
    for (int i = 20; i < 256; i++) {
        if (i != 32 && i != 33 && i != 0x80) {  // Кроме IRQ и syscall
            set_idt_gate(i, (uint64_t)isr_stub_default, 0x08, 0x8E);
        }
    }

    // Устанавливаем специальный обработчик для IRQ0 (таймер)
    set_idt_gate(32, (uint64_t)irq0_handler, 0x08, 0x8E);
    
    // Устанавливаем специальный обработчик для IRQ1 (клавиатура)
    set_idt_gate(33, (uint64_t)irq1_handler, 0x08, 0x8E);
    
    // Устанавливаем обработчик для системных вызовов (INT 0x80)
    // Флаги 0xEE: Present, Ring 3, Interrupt Gate
    set_idt_gate(0x80, (uint64_t)syscall_entry, 0x08, 0xEE);

    lidt(&idtp);
}
