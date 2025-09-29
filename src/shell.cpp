extern "C" {
    #include <stdint.h>
    #include "vga.h"
    #include "keyboard/input.h" 
    #include "handlers/timer.h"
    #include "std/string.h"
    #include "std/stddef.h"
    #include "shell.h"
    #include "memory/pmm.h"
    #include "memory/vmm.h"
    #include "process/pcb.h"
    #include "process/scheduler.h" 
    #include "process/test_processes.h"
    #include "userspace/user_programs.h"
    #include "gdt/gdt.h"
    #include "syscall/syscall.h"

    // Forward declaration from kernel.c
    extern uint64_t parse_multiboot2_memory_map(void *mbi);
    // Forward declaration for Buddy Allocator debug info
    extern buddy_block_t *free_lists[MAX_ORDER + 1];
    // Forward declaration for heap debug info
    extern uint64_t vmm_get_heap_info(uint64_t *start, uint64_t *current, uint64_t *end);
}

static void show_help(void);

// === Basic System Commands ===

static void cmd_help(void *mbi) {
    (void)mbi;
    show_help();
}

static void cmd_clear(void *mbi) {
    (void)mbi;
    for (int i = 0; i < 80 * 25; i++) {
        VGA[i] = (' ' | (current_color << 8));
    }
    row = 0;
    col = 0;
}

static void cmd_ticks(void *mbi) {
    (void)mbi;
    puts_color("System ticks: ", COLOR_INFO);
    puts_uint64_fg(ticks, VGA_COLOR_WHITE);
    puts("\n");
}

// === Memory Management Commands ===

// Consolidated command to show all memory-related information
static void cmd_meminfo(void *mbi) {
    (void)mbi;
    uint64_t total_memory = parse_multiboot2_memory_map(mbi);

    puts_color("--- Memory Subsystem Status ---\n", COLOR_INFO);

    // Total Memory
    puts_color("[Total Usable Memory]\n", COLOR_INFO);
    puts_color("  ", COLOR_TEXT);
    puts_uint64_fg(total_memory / (1024 * 1024), VGA_COLOR_WHITE);
    puts(" MB\n");

    // PMM Status
    puts_color("[Physical Memory Manager]\n", COLOR_INFO);
    puts_color("  Free: ", COLOR_TEXT);
    puts_uint64_fg(pmm_get_free_memory() / 1024, VGA_COLOR_GREEN);
    puts(" KB\n");
    puts_color("  Used: ", COLOR_TEXT);
    puts_uint64_fg(pmm_get_used_memory() / 1024, VGA_COLOR_LIGHT_BROWN);
    puts(" KB\n");

    // Kernel Heap Status
    puts_color("[Kernel Heap]\n", COLOR_INFO);
    uint64_t start, current, end;
    if (vmm_is_initialized() && vmm_get_heap_info(&start, &current, &end)) {
        puts_color("  Start: 0x", COLOR_TEXT); puts_hex64(start); puts("\n");
        puts_color("  End:   0x", COLOR_TEXT); puts_hex64(end); puts("\n");
        puts_color("  Used:  ", COLOR_TEXT); puts_uint64((current - start) / 1024); puts(" KB\n");
    } else {
        puts_color("  Heap is not initialized.\n", COLOR_ERROR);
    }
}

// Memory subsystem test command
static void cmd_test_mem(void *mbi) {
    (void)mbi;
    puts_color("--- Running Memory Tests ---\n", COLOR_INFO);

    // Test PMM
    puts_color("[PMM Test]\n", COLOR_INFO);
    uint64_t page1 = pmm_alloc_page();
    uint64_t page2 = pmm_alloc_page();
    if (page1 && page2) {
        puts_color("  Allocation:  SUCCESS\n", COLOR_SUCCESS);
        pmm_free_page(page1);
        pmm_free_page(page2);
        puts_color("  Freeing:     SUCCESS\n", COLOR_SUCCESS);
    } else {
        puts_color("  Allocation:  FAILED\n", COLOR_ERROR);
    }

    // Test Kernel Malloc
    puts_color("[Kernel Malloc Test]\n", COLOR_INFO);
    void *ptr1 = kmalloc(64);
    void *ptr2 = kmalloc_aligned(256, 64);
    if (ptr1 && ptr2) {
        puts_color("  Allocation:  SUCCESS\n", COLOR_SUCCESS);
        strcpy((char*)ptr1, "Test string");
        puts_color("  Write test:  SUCCESS (string='", COLOR_SUCCESS);
        puts((char*)ptr1);
        puts("')\n");
    } else {
        puts_color("  Allocation:  FAILED\n", COLOR_ERROR);
    }
    puts_color("--- Memory tests completed ---\n", COLOR_INFO);
}


// === Process & Scheduler Commands ===

static void cmd_ps(void *mbi) {
    (void)mbi;
    puts_color("--- Process List ---\n", COLOR_INFO);
    debug_process_table();
}

// Create all test processes (kernel and user) with one command
static void cmd_proc_init(void *mbi) {
    (void)mbi;
    puts_color("Creating kernel test processes...\n", COLOR_INFO);
    create_test_processes();
    puts_color("Creating user space test processes...\n", COLOR_INFO);
    create_user_processes();
    puts_color("All test processes created.\n", COLOR_SUCCESS);
}

static void cmd_sched_start(void *mbi) {
    (void)mbi;
    enable_multitasking();
    puts_color("Automatic multitasking ENABLED.\n", COLOR_SUCCESS);
    puts_color("Scheduler will run on timer ticks.\n", COLOR_INFO);
}

static void cmd_sched_stop(void *mbi) {
    (void)mbi;
    disable_multitasking();
    puts_color("Automatic multitasking DISABLED.\n", COLOR_WARNING);
}


// === Command Registry ===
static shell_command_t commands[] = {
    {"help",        "Show this help message", cmd_help},
    {"clear",       "Clear the screen", cmd_clear},
    {"ticks",       "Show current system ticks", cmd_ticks},
    {"meminfo",     "Show detailed memory status (PMM, Heap)", cmd_meminfo},
    {"test_mem",    "Run tests for PMM and kmalloc", cmd_test_mem},
    {"proc_init",   "Create all kernel and user test processes", cmd_proc_init},
    {"ps",          "List all active processes", cmd_ps},
    {"sched_start", "Enable automatic (preemptive) multitasking", cmd_sched_start},
    {"sched_stop",  "Disable automatic multitasking", cmd_sched_stop},
    {nullptr, nullptr, nullptr}
};

static void show_help(void) {
    puts_color("Available commands:\n", COLOR_INFO);
    for (int i = 0; commands[i].name != NULL; i++) {
        puts("  ");
        puts_color(commands[i].name, COLOR_SUCCESS);
        // Padding for alignment
        for (size_t p = 0; p < 14 - strlen(commands[i].name); ++p) {
            puts(" ");
        }
        puts("- ");
        puts(commands[i].description);
        puts("\n");
    }
}

void run_shell(void *mbi) {
    char input_buffer[256];
    
    puts_color("-*- Mini Shell -*-\n", COLOR_INFO);
    puts_color("Type 'help' for a list of commands.\n", COLOR_TEXT);
    
    while(1) {
        puts_color("shell> ", COLOR_WARNING);
        char* result = gets(input_buffer);

        if (result != NULL && strlen(result) > 0) {
            bool command_found = false;
            for (int i = 0; commands[i].name != NULL; i++) {
                if (strcmp(result, commands[i].name) == 0) {
                    commands[i].handler(mbi);
                    command_found = true;
                    break;
                }
            }
            
            if (!command_found) {
                puts_color("Unknown command: '", COLOR_ERROR);
                puts_fg(result, VGA_COLOR_RED);
                puts("'\n");
            }
        }
    }
}