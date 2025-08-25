#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef void (*isr_t)(void);

// Основные функции
void isr_handler(uint8_t n);
void register_interrupt_handler(uint8_t n, isr_t handler);

// Assembly заглушки
extern void isr_stub_default();
extern void irq0_handler();
extern void irq1_handler();
extern void syscall_entry(); // Точка входа для syscall

// Макрос для легкого добавления новых IRQ обработчиков
#define DECLARE_IRQ_HANDLER(num) extern void irq##num##_handler();

// Можно легко добавить новые IRQ обработчики
DECLARE_IRQ_HANDLER(2)  // IRQ2
DECLARE_IRQ_HANDLER(3)  // IRQ3
// и т.д.

#endif
