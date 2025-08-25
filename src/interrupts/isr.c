#include "isr.h"
#include "irq.h"
#include "io.h"
#include "vga.h"

static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(uint8_t n) {
    if (interrupt_handlers[n]) {
        interrupt_handlers[n]();
    } else {
        puts("Unhandled interrupt: ");
        puts_hex64(n);
        puts("\n");
    }
}

// Общая функция сохранения контекста
__asm__(
    ".macro SAVE_CONTEXT\n"
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
    ".endm\n"

    ".macro RESTORE_CONTEXT\n"
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
    ".endm\n"

    ".macro ISR_STUB name, int_num\n"
    ".global \\name\n"
    "\\name:\n"
    "    SAVE_CONTEXT\n"
    "    movq $\\int_num, %rdi\n"
    "    call isr_handler\n"
    "    RESTORE_CONTEXT\n"
    "    iretq\n"
    ".endm\n"

    // Генерируем заглушки с помощью макросов
    "ISR_STUB isr_stub_default, 0\n"
    "ISR_STUB irq0_handler, 32\n"
    "ISR_STUB irq1_handler, 33\n"
);
