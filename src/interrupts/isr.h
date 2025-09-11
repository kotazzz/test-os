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

// ISR заглушки для исключений (0-31)
extern void isr0();   // Division by Zero
extern void isr1();   // Debug
extern void isr2();   // NMI
extern void isr3();   // Breakpoint
extern void isr4();   // Overflow
extern void isr5();   // Bound Range Exceeded
extern void isr6();   // Invalid Opcode
extern void isr7();   // Device Not Available
extern void isr8();   // Double Fault
extern void isr9();   // Coprocessor Segment Overrun
extern void isr10();  // Invalid TSS
extern void isr11();  // Segment Not Present
extern void isr12();  // Stack Segment Fault
extern void isr13();  // General Protection Fault
extern void isr14();  // Page Fault
extern void isr15();  // Reserved
extern void isr16();  // x87 FPU Exception
extern void isr17();  // Alignment Check
extern void isr18();  // Machine Check
extern void isr19();  // SIMD Exception

// Макрос для легкого добавления новых IRQ обработчиков
#define DECLARE_IRQ_HANDLER(num) extern void irq##num##_handler();

// Можно легко добавить новые IRQ обработчики
DECLARE_IRQ_HANDLER(2)  // IRQ2
DECLARE_IRQ_HANDLER(3)  // IRQ3
// и т.д.

#endif
