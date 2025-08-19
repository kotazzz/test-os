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
    
    puts_color("Mini Shell - Available commands:\n", COLOR_INFO);
    puts("  ");
    puts_color("ticks", COLOR_SUCCESS);
    puts("  - Show system ticks\n");
    puts("  ");
    puts_color("memory", COLOR_SUCCESS);
    puts(" - Show memory information\n");
    puts("  ");
    puts_color("help", COLOR_SUCCESS);
    puts("   - Show this help\n");
    puts_color("shell> ", COLOR_WARNING);
    
    while(1) {
        char* result = gets(input_buffer);
        if (result != NULL) {
            if (strcmp(result, "ticks")) {
                puts_color("System ticks: ", COLOR_INFO);
                puts_uint64_fg(ticks, VGA_COLOR_WHITE);
                puts("\n");
            }
            else if (strcmp(result, "memory")) {
                uint64_t total_memory = parse_multiboot2_memory_map(mbi);
                puts_color("Memory Information:\n", COLOR_INFO);
                puts_color("  Total usable: ", COLOR_TEXT);
                puts_uint64_fg(total_memory, VGA_COLOR_WHITE);
                puts(" bytes (");
                puts_uint64_fg(total_memory / 1024, VGA_COLOR_WHITE);
                puts(" KB, ");
                puts_uint64_fg(total_memory / (1024 * 1024), VGA_COLOR_WHITE);
                puts(" MB)\n");
            }
            else if (strcmp(result, "help")) {
                puts_color("Available commands:\n", COLOR_INFO);
                puts("  ");
                puts_color("ticks", COLOR_SUCCESS);
                puts("  - Show system ticks\n");
                puts("  ");
                puts_color("memory", COLOR_SUCCESS);
                puts(" - Show memory information\n");
                puts("  ");
                puts_color("help", COLOR_SUCCESS);
                puts("   - Show this help\n");
            }
            else {
                puts_color("Unknown command: ", COLOR_ERROR);
                puts_fg(result, VGA_COLOR_RED);
                puts_color("\nType 'help' for available commands\n", COLOR_TEXT);
            }
            
            puts_color("shell> ", COLOR_WARNING);
        }
    }
}