#include "isr.h"
#include "irq.h"
#include "io.h"
#include "vga.h"
#include "../process/pcb.h"

static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(uint8_t n) {
    if (interrupt_handlers[n]) {
        interrupt_handlers[n]();
    } else {
        // Различаем системные исключения и IRQ
        if (n < 32) {
            puts_color("CPU Exception ", COLOR_ERROR);
            puts_hex64(n);
            puts(": ");

            // Описания основных исключений процессора
            switch(n) {
                case 0: puts("Division by Zero"); break;
                case 1: puts("Debug Exception"); break;
                case 2: puts("NMI"); break;
                case 3: puts("Breakpoint"); break;
                case 4: puts("Overflow"); break;
                case 5: puts("Bound Range Exceeded"); break;
                case 6: puts("Invalid Opcode"); break;
                case 7: puts("Device Not Available"); break;
                case 8: puts("Double Fault"); break;
                case 10: puts("Invalid TSS"); break;
                case 11: puts("Segment Not Present"); break;
                case 12: puts("Stack Segment Fault"); break;
                case 13: puts("General Protection Fault"); break;
                case 14: puts("Page Fault"); break;
                default: puts("Unknown Exception"); break;
            }
            puts("\n");

            // Добавляем отладочную информацию для user mode процессов
            pcb_t* current = get_current_process();
            if (current) {
                puts("Current process PID: ");
                puts_hex64(current->pid);
                puts(", is_user: ");
                puts_hex64(current->is_user_process);
                puts("\n");

                // For user processes, terminate the process instead of halting the system
                if (current->is_user_process) {
                    puts_color("User process caused exception - terminating process\n", COLOR_WARNING);
                    extern void terminate_process(uint32_t pid);
                    terminate_process(current->pid);
                    
                    // Switch to next available process or idle
                    extern void run_scheduler();
                    run_scheduler();
                    return; // Don't halt the system
                }

                // Log RIP and RSP for debugging
                uint64_t rip, rsp;
                __asm__ volatile ("lea (%%rip), %0" : "=r"(rip));
                __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));
                puts("RIP: ");
                puts_hex64(rip);
                puts("\nRSP: ");
                puts_hex64(rsp);
                puts("\n");
            }

            // Останавливаем систему при критических исключениях (только для kernel процессов)
            if (n == 8 || n == 13 || n == 14) {
                puts("CRITICAL ERROR - System halted\n");
                __asm__ volatile ("cli; hlt");
            }
        } else {
            puts("Unhandled IRQ: ");
            puts_hex64(n - 32);
            puts("\n");
        }
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
