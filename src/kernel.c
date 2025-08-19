#include <stdint.h>
#include "vga.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "keyboard/input.h"
#include "handlers/timer.h"
#include "shell.h"
#include "multiboot/multiboot.h"

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

    // Initialize interrupts
    init_idt();
    init_irq();
    
    // Enable interrupts
    __asm__ volatile ("sti");

    puts("Interrupts enabled. System ready.\n");
    puts("Type something and press Enter:\n");
    
    // Start the mini shell
    run_shell(mbi);
}
