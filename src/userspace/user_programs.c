#include "user_programs.h"
#include "../syscall/syscall.h"
#include "../process/pcb.h"
#include "../vga.h"

// Simple test user program that doesn't use syscalls or any kernel functions
void simple_user_test(void) {
    // Very minimal test with proper termination
    // Use inline assembly to safely exit instead of return
    __asm__ volatile (
        "movq $0, %%rdi\n"      // exit code = 0
        "movq $0, %%rax\n"      // syscall number = SYS_EXIT
        "int $0x80\n"           // trigger syscall
        :
        :
        : "rax", "rdi"
    );
    
    // This should never be reached, but just in case
    while(1) {
        __asm__ volatile ("hlt");
    }
}

// User space program A - with proper x64 syscalls (commented out for safety)
void user_program_a(void) {
    // Example of how to use syscalls in user mode:
    // syscall_write("Hello from user program A!\n");
    // syscall_yield();
    // syscall_exit(0);
    
    // For now, minimal test without syscalls
    return;
}

// User space program B  
void user_program_b(void) {
    // Example of how to use fast x64 syscalls:
    // syscall_write_fast("Hello from user program B!\n");
    // syscall_yield_fast();
    // syscall_exit_fast(0);
    
    // For now, minimal test without syscalls
    return;
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
