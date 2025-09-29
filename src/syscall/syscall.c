#include "syscall.h"
#include "../vga.h"
#include "../process/pcb.h"
#include "../interrupts/isr.h" // Для register_interrupt_handler
#include "../std/string.h"

// Точка входа для syscall из ассемблера (x64 optimized)
__asm__(
    ".global syscall_entry\n"
    "syscall_entry:\n"
    // Сохраняем контекст (все 64-битные регистры в том порядке, как они определены в структуре registers)
    "    pushq %rbp\n"
    "    pushq %rax\n"    // Syscall number
    "    pushq %rbx\n"
    "    pushq %rcx\n"
    "    pushq %rdx\n"    // arg3
    "    pushq %rsi\n"    // arg2
    "    pushq %rdi\n"    // arg1
    "    pushq %r8\n"
    "    pushq %r9\n"
    "    pushq %r10\n"
    "    pushq %r11\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    
    // Передаем указатель на сохраненный контекст как первый аргумент
    "    movq %rsp, %rdi\n"
    
    // Выравнивание стека на 16 байт (требование x64 ABI)
    "    pushq %rsp\n"    // Сохраняем текущий RSP
    "    andq $-16, %rsp\n"
    
    "    call syscall_handler_main\n" // Вызываем C-обработчик
    
    // Восстанавливаем выравненный стек
    "    popq %rsp\n"
    
    // Восстанавливаем контекст (порядок обратный сохранению)
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %r11\n"
    "    popq %r10\n"
    "    popq %r9\n"
    "    popq %r8\n"
    "    popq %rdi\n"
    "    popq %rsi\n"
    "    popq %rdx\n"
    "    popq %rcx\n"
    "    popq %rbx\n"
    "    popq %rax\n"     // RAX содержит результат
    "    popq %rbp\n"
    "    iretq\n"         // 64-битный возврат
);

// C-функция обработчика syscall
void syscall_handler_main(struct registers *regs) {
    uint64_t syscall_num = regs->rax;
    uint64_t arg1 = regs->rdi;
    // uint64_t arg2 = regs->rsi; // Reserved for future use
    // uint64_t arg3 = regs->rdx; // Reserved for future use
    uint64_t result = 0;
    
    switch(syscall_num) {
        case SYS_EXIT: {
            pcb_t* current = get_current_process();
            if (current) {
                terminate_process(current->pid); // This sets current_process to NULL
            }
            
            // We are now in the syscall handler and cannot return to the terminated process.
            // We must find a new process to run and switch to it directly.
            extern pcb_t* get_next_process(void);
            extern void restore_next_context(pcb_t*);
            
            pcb_t* next = get_next_process();
            if (next) {
                set_current_process(next);
                restore_next_context(next); // This function will not return
            }
            
            // If no other processes exist, halt the system.
            puts("All processes terminated. System halting.\n");
            __asm__ volatile ("cli; hlt");
            break; // Unreachable
        }
            
        case SYS_WRITE:
            // TODO: Проверка, что arg1 указывает на валидную память в user-space
            // Для простоты, пока просто выводим, но в будущем нужна проверка
            {
                const char* str = (const char*)arg1;
                // В реальной системе нужно скопировать строку в буфер ядра
                // и проверить каждый байт на доступность.
                // Пока что просто выведем.
                puts(str);
                result = strlen(str);
            }
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
    
    // Возвращаем результат в RAX через структуру регистров
    regs->rax = result;
}

void init_syscalls(void) {
    // Register syscall handler for interrupt 0x80 (traditional method)
    extern void register_interrupt_handler(uint8_t n, void (*handler)(void));
    register_interrupt_handler(0x80, (void(*)(void))syscall_entry);
    puts("Syscall interface initialized (INT 0x80 + SYSCALL support)\n");
    
    // TODO: Setup MSRs for SYSCALL/SYSRET mechanism
    // This would require setting up IA32_LSTAR, IA32_STAR, IA32_FMASK, IA32_EFER.SCE
    // For now, we use the compatible INT 0x80 method
    puts("Note: Using INT 0x80 for compatibility. SYSCALL instruction available but not configured.\n");
}

// User space syscall wrappers (these will be called from user programs)
int syscall_exit(int code) {
    long result;
    __asm__ volatile (
        "movq %1, %%rdi\n"     // arg1 = code
        "movq %2, %%rax\n"     // syscall number
        "int $0x80\n"         // trigger syscall
        "movq %%rax, %0\n"     // get result (64-bit)
        : "=r"(result)
        : "r"((long)code), "i"(SYS_EXIT)
        : "rax", "rdi"
    );
    return (int)result;
}

int syscall_write(const char* str) {
    long result;
    __asm__ volatile (
        "movq %1, %%rdi\n"     // arg1 = string
        "movq %2, %%rax\n"     // syscall number  
        "int $0x80\n"         // trigger syscall
        "movq %%rax, %0\n"     // get result (64-bit)
        : "=r"(result)
        : "r"((long)str), "i"(SYS_WRITE)
        : "rax", "rdi"
    );
    return (int)result;
}

int syscall_yield(void) {
    long result;
    __asm__ volatile (
        "movq %1, %%rax\n"     // syscall number
        "int $0x80\n"         // trigger syscall  
        "movq %%rax, %0\n"     // get result (64-bit)
        : "=r"(result)
        : "i"(SYS_YIELD)
        : "rax"
    );
    return (int)result;
}

// Fast x64 syscall implementations using SYSCALL instruction
// Note: These require proper MSR setup for SYSCALL/SYSRET mechanism

int syscall_exit_fast(int code) {
    long result;
    __asm__ volatile (
        "movq %1, %%rdi\n"     // arg1 = code  
        "movq %2, %%rax\n"     // syscall number
        "syscall\n"           // fast x64 syscall
        "movq %%rax, %0\n"     // get result
        : "=r"(result)
        : "r"((long)code), "i"(SYS_EXIT)
        : "rax", "rdi", "rcx", "r11"  // SYSCALL clobbers RCX and R11
    );
    return (int)result;
}

int syscall_write_fast(const char* str) {
    long result;
    __asm__ volatile (
        "movq %1, %%rdi\n"     // arg1 = string
        "movq %2, %%rax\n"     // syscall number
        "syscall\n"           // fast x64 syscall
        "movq %%rax, %0\n"     // get result
        : "=r"(result)
        : "r"((long)str), "i"(SYS_WRITE)
        : "rax", "rdi", "rcx", "r11"  // SYSCALL clobbers RCX and R11
    );
    return (int)result;
}

int syscall_yield_fast(void) {
    long result;
    __asm__ volatile (
        "movq %1, %%rax\n"     // syscall number
        "syscall\n"           // fast x64 syscall
        "movq %%rax, %0\n"     // get result
        : "=r"(result)
        : "i"(SYS_YIELD)
        : "rax", "rcx", "r11"  // SYSCALL clobbers RCX and R11
    );
    return (int)result;
}
