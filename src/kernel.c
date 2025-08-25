#include <stdint.h>
#include "vga.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "keyboard/input.h"
#include "handlers/timer.h"
#include "shell.h"
#include "multiboot/multiboot.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "process/test_processes.h"
#include "process/pcb.h"
#include "process/scheduler.h"
#include "gdt/gdt.h"
#include "syscall/syscall.h"

void kmain(void *mbi){
    row = 0;
    col = 0;
    
    // Clear screen
    for(int i = 0; i < 80 * 25; i++) 
        VGA[i] = (' ' | (0x07 << 8));
    
    puts("TOS - Tiny Operating System\n");
    puts("64-bit kernel with keyboard support\n");
    
    // Display multiboot information
    print_multiboot_info(mbi);

    // Initialize memory management
    puts("Initializing memory management...\n");
    pmm_init(mbi);  // Physical Memory Manager
    buddy_init();
    vmm_init();     // Virtual Memory Manager

    // Initialize process system
    puts("Initializing process system...\n");
    init_process_system();

    // Initialize GDT for user/kernel separation
    puts("Setting up user/kernel separation...\n");
    init_gdt();
    
    // Initialize system calls
    init_syscalls();

    // Initialize interrupts
    puts("Initializing interrupts...\n");
    init_idt();
    init_irq();
    
    // Initialize timer BEFORE enabling interrupts
    puts("Initializing timer...\n");
    init_timer(100); // 100 Hz timer
    
    puts("Enabling interrupts...\n");
    // Enable interrupts
    __asm__ volatile ("sti");
    
    puts("Interrupts enabled. System ready.\n");
    puts("Kernel/User space separation enabled!\n");
    puts("Use 'create' for kernel processes, 'user-create' for user processes\n");
    
    puts("Starting shell...\n");
    // Start the mini shell
    run_shell(mbi);
}
