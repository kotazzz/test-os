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

    // Заполняем все дескрипторы дефолтным обработчиком
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, (uint64_t)isr_stub_default, 0x08, 0x8E);
    }

    // Устанавливаем специальный обработчик для IRQ0 (таймер)
    set_idt_gate(32, (uint64_t)irq0_handler, 0x08, 0x8E);
    
    // Устанавливаем специальный обработчик для IRQ1 (клавиатура)
    set_idt_gate(33, (uint64_t)irq1_handler, 0x08, 0x8E);

    lidt(&idtp);
}
