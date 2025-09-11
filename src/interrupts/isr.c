#include "isr.h"
#include "irq.h"
#include "../io.h"
#include "../vga.h"
#include "../process/pcb.h"
#include <stddef.h>

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
                case 13: 
                    puts("General Protection Fault");
                    // For GPF, print error code which is on stack
                    uint64_t error_code;
                    __asm__ volatile ("mov 16(%%rbp), %0" : "=r"(error_code));
                    puts(" (Error Code: ");
                    puts_hex64(error_code);
                    puts(")");
                    break;
                case 14: 
                    puts("Page Fault");
                    // For Page Fault, print CR2 (faulting address)
                    uint64_t cr2;
                    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
                    puts(" (Address: ");
                    puts_hex64(cr2);
                    puts(")");
                    break;
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
                    
                    // Clear current process to prevent re-running the same process
                    extern pcb_t* get_current_process();
                    extern void set_current_process(pcb_t*);
                    set_current_process(NULL);
                    
                    // Try to switch to next available process
                    extern void run_scheduler();
                    run_scheduler();
                    
                    // If scheduler didn't find any process, we're in trouble
                    puts_color("Warning: No processes available after exception handling\n", COLOR_WARNING);
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
