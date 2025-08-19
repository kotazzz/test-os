extern "C" {
    #include <stdint.h>
    #include "vga.h"
    #include "keyboard/input.h" 
    #include "handlers/timer.h"
    #include "std/string.h"
    #include "std/stddef.h"
    #include "shell.h"
    #include "std/random.h"

    // Forward declaration - эта функция остается в kernel.c
    extern uint64_t parse_multiboot2_memory_map(void *mbi);
}

static void show_help(void);


// Show cat
static void cmd_cat(void *mbi) {
    puts_color(" _._     _,-'\"\"`-._     \n", COLOR_INFO);
    puts_color("(,-.`._,'(       |\\`-/| \n", COLOR_INFO);
    puts_color("    `-.-' \\ )-`( , o o) \n", COLOR_INFO);
    puts_color("          `-    \\`_`\"'- \n", COLOR_INFO);
    puts("\nThis is a cat!\n");
}

static void cmd_random(void *mbi) {
    puts_color("Random number: ", COLOR_INFO);
    xorshift_state_t rng;
    xorshift_seed(&rng, ticks);
    puts_uint64_fg(xorshift_next(&rng), VGA_COLOR_WHITE);
    puts("\n");
}

// Command handlers
static void cmd_ticks(void *mbi) {
    puts_color("System ticks: ", COLOR_INFO);
    puts_uint64_fg(ticks, VGA_COLOR_WHITE);
    puts("\n");
}

static void cmd_memory(void *mbi) {
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

static void cmd_help(void *mbi) {
    show_help();
}

static void cmd_clear(void *mbi) {
    for (int i = 0; i < 80 * 25; i++) {
        VGA[i] = (' ' | (current_color << 8));
    }
    row = 0;
    col = 0;
}

// Command registry
static shell_command_t commands[] = {
    {"cat", "Show pretty cat", cmd_cat},
    {"random", "Show random number", cmd_random},
    {"ticks", "Show system ticks", cmd_ticks},
    {"memory", "Show memory information", cmd_memory},
    {"help", "Show this help", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
};

static void show_help(void) {
    puts_color("Available commands:\n", COLOR_INFO);
    for (int i = 0; commands[i].name != NULL; i++) {
        puts("  ");
        puts_color(commands[i].name, COLOR_SUCCESS);
        puts(" - ");
        puts(commands[i].description);
        puts("\n");
    }
}

void run_shell(void *mbi) {
    char input_buffer[256];
    
    puts_color("Mini Shell - ", COLOR_INFO);
    show_help();
    puts_color("shell> ", COLOR_WARNING);
    
    while(1) {
        char* result = gets(input_buffer);
        if (result != NULL) {
            // Find and execute command
            bool command_found = false;
            for (int i = 0; commands[i].name != NULL; i++) {
                if (strcmp(result, commands[i].name)) {
                    commands[i].handler(mbi);
                    command_found = true;
                    break;
                }
            }
            
            if (!command_found) {
                puts_color("Unknown command: ", COLOR_ERROR);
                puts_fg(result, VGA_COLOR_RED);
                puts_color("\nType 'help' for available commands\n", COLOR_TEXT);
            }
            
            puts_color("shell> ", COLOR_WARNING);
        }
    }
}