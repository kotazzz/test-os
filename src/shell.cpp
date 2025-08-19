extern "C" {
    #include <stdint.h>
    #include "vga.h"
    #include "keyboard/input.h" 
    #include "handlers/timer.h"
    #include "std/string.h"
    #include "std/stddef.h"
    #include "shell.h"
    
    // Forward declaration - эта функция остается в kernel.c
    extern uint64_t parse_multiboot2_memory_map(void *mbi);
}

void run_shell(void *mbi) {
    char input_buffer[256];
    
    puts("Mini Shell - Available commands:\n");
    puts("  ticks  - Show system ticks\n");
    puts("  memory - Show memory information\n");
    puts("  help   - Show this help\n");
    puts("shell> ");
    
    while(1) {
        char* result = gets(input_buffer);
        if (result != NULL) {
            if (strcmp(result, "ticks")) {
                puts("System ticks: ");
                puts_uint64(ticks);
                puts("\n");
            }
            else if (strcmp(result, "memory")) {
                uint64_t total_memory = parse_multiboot2_memory_map(mbi);
                puts("Memory Information:\n");
                puts("  Total usable: ");
                puts_uint64(total_memory);
                puts(" bytes (");
                puts_uint64(total_memory / 1024);
                puts(" KB, ");
                puts_uint64(total_memory / (1024 * 1024));
                puts(" MB)\n");
            }
            else if (strcmp(result, "help")) {
                puts("Available commands:\n");
                puts("  ticks  - Show system ticks\n");
                puts("  memory - Show memory information\n");
                puts("  help   - Show this help\n");
            }
            else {
                puts("Unknown command: ");
                puts(result);
                puts("\nType 'help' for available commands\n");
            }
            
            puts("shell> ");
        }
    }
}