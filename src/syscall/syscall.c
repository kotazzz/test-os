#include "syscall.h"
#include "../vga.h"
#include "../process/pcb.h"
#include "../interrupts/isr.h" // Для register_interrupt_handler
#include "../std/string.h"

// Точка входа для syscall из ассемблера
__asm__(
    ".global syscall_entry\n"
    "syscall_entry:\n"
    "    push %rbp\n"
    "    mov %rsp, %rbp\n"
    "    push %rax\n" // Сохраняем регистры, которые могут быть изменены
    "    push %rbx\n"
    "    push %rcx\n"
    "    push %rdx\n"
    "    push %rsi\n"
    "    push %rdi\n"
    "    push %r8\n"
    "    push %r9\n"
    "    push %r10\n"
    "    push %r11\n"
    "    push %r12\n"
    "    push %r13\n"
    "    push %r14\n"
    "    push %r15\n"
    
    "    call syscall_handler_main\n" // Вызываем C-обработчик
    
    "    pop %r15\n"
    "    pop %r14\n"
    "    pop %r13\n"
    "    pop %r12\n"
    "    pop %r11\n"
    "    pop %r10\n"
    "    pop %r9\n"
    "    pop %r8\n"
    "    pop %rdi\n"
    "    pop %rsi\n"
    "    pop %rdx\n"
    "    pop %rcx\n"
    "    pop %rbx\n"
    "    pop %rax\n"
    "    mov %rbp, %rsp\n"
    "    pop %rbp\n"
    "    iretq\n"
);

// C-функция обработчика syscall
void syscall_handler_main(void) {
    uint64_t syscall_num, arg1, arg2, arg3, result = 0;
    
    // Получаем аргументы syscall из регистров
    __asm__ volatile (
        "mov %%rax, %0\n"
        "mov %%rdi, %1\n" 
        "mov %%rsi, %2\n"
        "mov %%rdx, %3\n"
        : "=r"(syscall_num), "=r"(arg1), "=r"(arg2), "=r"(arg3)  // Используем "=r" вместо "=m"
        :
        : "memory"
    );
    
    switch(syscall_num) {
        case SYS_EXIT:
            if (get_current_process()) {
                terminate_process(get_current_process()->pid);
            }
            yield(); // Переключаемся на другой процесс
            break;
            
        case SYS_WRITE:
            puts((const char*)arg1);
            result = strlen((const char*)arg1);
            break;
            
        case SYS_YIELD:
            yield();
            break;
            
        default:
            puts("Unknown syscall: ");
            puts_hex64(syscall_num);
            puts("\n");
            result = -1;
            break;
    }
    
    // Возвращаем результат в RAX
    __asm__ volatile ("mov %0, %%rax" : : "r"(result) : "rax");
}

void init_syscalls(void) {
    puts("Syscall interface initialized (handled by INT 0x80)\n");
}

// User space syscall wrappers (these will be called from user programs)
int syscall_exit(int code) {
    int result;
    __asm__ volatile (
        "mov %1, %%rdi\n"     // arg1 = code
        "mov %2, %%rax\n"     // syscall number
        "int $0x80\n"         // trigger syscall
        "mov %%eax, %0\n"     // get result
        : "=m"(result)
        : "m"(code), "i"(SYS_EXIT)
        : "rax", "rdi"
    );
    return result;
}

int syscall_write(const char* str) {
    int result;
    __asm__ volatile (
        "mov %1, %%rdi\n"     // arg1 = string
        "mov %2, %%rax\n"     // syscall number  
        "int $0x80\n"         // trigger syscall
        "mov %%eax, %0\n"     // get result
        : "=m"(result)
        : "m"(str), "i"(SYS_WRITE)
        : "rax", "rdi"
    );
    return result;
}

int syscall_yield(void) {
    int result;
    __asm__ volatile (
        "mov %1, %%rax\n"     // syscall number
        "int $0x80\n"         // trigger syscall  
        "mov %%eax, %0\n"     // get result
        : "=m"(result)
        : "i"(SYS_YIELD)
        : "rax"
    );
    return result;
}
