#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// System call numbers
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_YIELD   3

// System call handler
void init_syscalls(void);
void syscall_handler_main(void); // Переименовано

// User space syscall interface
int syscall_exit(int code);
int syscall_write(const char* str);
int syscall_yield(void);

#endif // SYSCALL_H
