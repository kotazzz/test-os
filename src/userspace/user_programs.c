#include "user_programs.h"
#include "../syscall/syscall.h"
#include "../process/pcb.h"
#include "../vga.h"

// Simple test user program that doesn't use syscalls or any kernel functions
void simple_user_test(void) {
    syscall_write("Hello from simple user process!\n");
    for(int i = 0; i < 5; i++) {
        syscall_write("User process yielding...\n");
        syscall_yield();
    }
    syscall_write("User process exiting.\n");
    syscall_exit(0);
    
    // Этот код никогда не должен выполниться
    __builtin_unreachable();
}

// User space program A - with proper x64 syscalls
void user_program_a(void) {
    syscall_write("User Program A: Running...\n");
    syscall_yield();
    syscall_write("User Program A: Exiting.\n");
    syscall_exit(1);
    __builtin_unreachable();
}

// User space program B  
void user_program_b(void) {
    syscall_write("User Program B: Running...\n");
    syscall_yield();
    syscall_write("User Program B: Exiting.\n");
    syscall_exit(2);
    __builtin_unreachable();
}

void create_user_processes(void) {
    puts_color("Creating user space processes...\n", COLOR_INFO);
    
    // Create only one simple test process to avoid flooding
    pcb_t* proc_test = create_user_process(simple_user_test);

    if (proc_test) {
        puts_color("User space process created successfully\n", COLOR_SUCCESS);
    } else {
        puts_color("Failed to create user space process\n", COLOR_ERROR);
    }
}
