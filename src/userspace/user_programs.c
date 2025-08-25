#include "user_programs.h"
#include "../syscall/syscall.h"
#include "../process/pcb.h"
#include "../vga.h"

// Simple test user program that doesn't use syscalls or any kernel functions
void simple_user_test(void) {
    // Very minimal test - just return immediately without any syscalls
    return;
}

// User space program A - safer version with inline assembly syscalls
void user_program_a(void) {
    // Minimal test without syscalls
    return;
}

// User space program B  
void user_program_b(void) {
    // Minimal test without syscalls
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
