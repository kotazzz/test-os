#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// System call numbers
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_YIELD   3

// Register context structure for syscalls
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rbp;
};

// System call handler
void init_syscalls(void);
void syscall_handler_main(struct registers *regs);

// User space syscall interface (INT 0x80 version - compatible)
int syscall_exit(int code);
int syscall_write(const char* str);
int syscall_yield(void);

// Fast x64 syscall interface (SYSCALL instruction - optional)
int syscall_exit_fast(int code);
int syscall_write_fast(const char* str);
int syscall_yield_fast(void);

#endif // SYSCALL_H
