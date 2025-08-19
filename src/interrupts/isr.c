#include "isr.h"
#include "irq.h"
#include "../io.h"
#include "../vga.h" // например, для вывода текста

static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(uint8_t n) {
    if (interrupt_handlers[n]) {
        interrupt_handlers[n]();
    } else {
        // puts("Unhandled interrupt: ");
        // puts_uint64(n);
        // puts("\n");
    }
}

// Assembly заглушки для обработки прерываний
__asm__(
    ".global isr_stub_default\n"
    "isr_stub_default:\n"
    "    push %rax\n"
    "    push %rbx\n" 
    "    push %rcx\n"
    "    push %rdx\n"
    "    push %rsi\n"
    "    push %rdi\n"
    "    push %rbp\n"
    "    push %r8\n"
    "    push %r9\n"
    "    push %r10\n"
    "    push %r11\n"
    "    push %r12\n"
    "    push %r13\n"
    "    push %r14\n"
    "    push %r15\n"
    "    movq $0, %rdi\n"          // Номер прерывания (пока 0)
    "    call isr_handler\n"
    "    pop %r15\n"
    "    pop %r14\n"
    "    pop %r13\n"
    "    pop %r12\n"
    "    pop %r11\n"
    "    pop %r10\n"
    "    pop %r9\n"
    "    pop %r8\n"
    "    pop %rbp\n"
    "    pop %rdi\n"
    "    pop %rsi\n"
    "    pop %rdx\n"
    "    pop %rcx\n"
    "    pop %rbx\n"
    "    pop %rax\n"
    "    iretq\n"
);

// Assembly заглушка для IRQ0 (таймер)
__asm__(
    ".global irq0_handler\n"
    "irq0_handler:\n"
    "    push %rax\n"
    "    push %rbx\n" 
    "    push %rcx\n"
    "    push %rdx\n"
    "    push %rsi\n"
    "    push %rdi\n"
    "    push %rbp\n"
    "    push %r8\n"
    "    push %r9\n"
    "    push %r10\n"
    "    push %r11\n"
    "    push %r12\n"
    "    push %r13\n"
    "    push %r14\n"
    "    push %r15\n"
    "    movq $32, %rdi\n"         // IRQ0 = прерывание 32
    "    call isr_handler\n"
    "    pop %r15\n"
    "    pop %r14\n"
    "    pop %r13\n"
    "    pop %r12\n"
    "    pop %r11\n"
    "    pop %r10\n"
    "    pop %r9\n"
    "    pop %r8\n"
    "    pop %rbp\n"
    "    pop %rdi\n"
    "    pop %rsi\n"
    "    pop %rdx\n"
    "    pop %rcx\n"
    "    pop %rbx\n"
    "    pop %rax\n"
    "    iretq\n"
);
