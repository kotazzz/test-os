extern "C" {
    #include <stdint.h>
    #include "vga.h"
    #include "keyboard/input.h" 
    #include "handlers/timer.h"
    #include "std/string.h"
    #include "std/stddef.h"
    #include "shell.h"
    #include "std/random.h"
    #include "memory/pmm.h"
    #include "memory/vmm.h"
    #include "process/pcb.h"
    #include "process/scheduler.h" 
    #include "process/test_processes.h"

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

// Memory management commands
static void cmd_pmm_info(void *mbi) {
    puts_color("Physical Memory Manager Status:\n", COLOR_INFO);
    puts_color("  Free memory: ", COLOR_TEXT);
    puts_uint64_fg(pmm_get_free_memory() / 1024, VGA_COLOR_GREEN);
    puts(" KB\n");
    puts_color("  Used memory: ", COLOR_TEXT);
    puts_uint64_fg(pmm_get_used_memory() / 1024, VGA_COLOR_LIGHT_BROWN);
    puts(" KB\n");
}

static void cmd_memmap(void *mbi) {
    puts_color("Physical Memory Map:\n", COLOR_INFO);
    pmm_dump_memory_map();
}

// Debug kernel heap command
extern uint64_t vmm_get_heap_info(uint64_t *start, uint64_t *current, uint64_t *end);

static void cmd_heap_info(void *mbi) {
    uint64_t start, current, end;
    vmm_get_heap_info(&start, &current, &end);
    
    puts_color("Kernel Heap Information:\n", COLOR_INFO);
    
    if (!vmm_is_initialized()) {
        puts_color("  VMM or Heap is not initialized!\n", COLOR_ERROR);
        return;
    }
    
    puts_color("  Start: 0x", COLOR_TEXT);
    puts_hex64(start); puts("\n");
    puts_color("  Current: 0x", COLOR_TEXT);
    puts_hex64(current); puts("\n");
    puts_color("  End: 0x", COLOR_TEXT);
    puts_hex64(end); puts("\n");
    puts_color("  Total size: ", COLOR_TEXT);
    puts_uint64((end - start) / (1024 * 1024)); puts(" MB\n");
    puts_color("  Used: ", COLOR_TEXT);
    puts_uint64((current - start) / 1024); puts(" KB\n");
    puts_color("  Available: ", COLOR_TEXT);
    puts_uint64((end - current) / 1024); puts(" KB\n");
}

static void cmd_test_all(void *mbi) {
    puts_color("Running all tests:\n", COLOR_INFO);

    // Test PMM
    puts_color("[PMM Test]\n", COLOR_INFO);
    uint64_t page1 = pmm_alloc_page();
    uint64_t page2 = pmm_alloc_page();
    uint64_t page3 = pmm_alloc_page();

    if (page1 && page2 && page3) {
        puts_color("  PMM Allocation: SUCCESS\n", COLOR_SUCCESS);
        puts("    Page 1: 0x"); puts_hex64(page1); puts("\n");
        puts("    Page 2: 0x"); puts_hex64(page2); puts("\n");
        puts("    Page 3: 0x"); puts_hex64(page3); puts("\n");

        pmm_free_page(page1);
        pmm_free_page(page2);
        pmm_free_page(page3);
        puts_color("  PMM Free: SUCCESS\n", COLOR_SUCCESS);
    } else {
        puts_color("  PMM Allocation: FAILED\n", COLOR_ERROR);
    }

    // Test Kernel Malloc
    puts_color("[Kernel Malloc Test]\n", COLOR_INFO);
    void *ptr1 = kmalloc(64);
    void *ptr2 = kmalloc(128);
    void *ptr3 = kmalloc_aligned(256, 64);

    if (ptr1 && ptr2 && ptr3) {
        puts_color("  Kernel Malloc: SUCCESS\n", COLOR_SUCCESS);
        puts("    ptr1 (64 bytes): 0x"); puts_hex64((uint64_t)ptr1); puts("\n");
        puts("    ptr2 (128 bytes): 0x"); puts_hex64((uint64_t)ptr2); puts("\n");
        puts("    ptr3 (256 bytes, 64-aligned): 0x"); puts_hex64((uint64_t)ptr3); puts("\n");

        char *test_str = (char*)ptr1;
        strcpy(test_str, "Hello from kmalloc!");
        puts_color("    Test string written: ", COLOR_TEXT);
        puts(test_str); puts("\n");
    } else {
        puts_color("  Kernel Malloc: FAILED\n", COLOR_ERROR);
    }

    // Test Heap Info
    puts_color("[Kernel Heap Info]\n", COLOR_INFO);
    uint64_t start, current, end;
    vmm_get_heap_info(&start, &current, &end);

    if (vmm_is_initialized()) {
        puts_color("  Heap Info: SUCCESS\n", COLOR_SUCCESS);
        puts("    Start: 0x"); puts_hex64(start); puts("\n");
        puts("    Current: 0x"); puts_hex64(current); puts("\n");
        puts("    End: 0x"); puts_hex64(end); puts("\n");
        puts("    Total size: "); puts_uint64((end - start) / (1024 * 1024)); puts(" MB\n");
        puts("    Used: "); puts_uint64((current - start) / 1024); puts(" KB\n");
        puts("    Available: "); puts_uint64((end - current) / 1024); puts(" KB\n");
    } else {
        puts_color("  Heap Info: FAILED (VMM not initialized)\n", COLOR_ERROR);
    }

    puts_color("All tests completed.\n", COLOR_INFO);
}

static void cmd_memory_info(void *mbi) {
    puts_color("Memory Information:\n", COLOR_INFO);

    // PMM Status
    puts_color("[Physical Memory Manager]\n", COLOR_INFO);
    puts_color("  Free memory: ", COLOR_TEXT);
    puts_uint64_fg(pmm_get_free_memory() / 1024, VGA_COLOR_GREEN);
    puts(" KB\n");
    puts_color("  Used memory: ", COLOR_TEXT);
    puts_uint64_fg(pmm_get_used_memory() / 1024, VGA_COLOR_LIGHT_BROWN);
    puts(" KB\n");

    // Buddy Allocator Status
    puts_color("[Buddy Allocator]\n", COLOR_INFO);
    for (int i = 0; i <= MAX_ORDER; i++) {
        puts("  Order "); puts_uint64(i); puts(": ");
        buddy_block_t *block = free_lists[i];
        int count = 0;
        while (block) {
            count++;
            block = block->next;
        }
        puts_uint64(count); puts(" blocks\n");
    }

    // Kernel Heap Status
    puts_color("[Kernel Heap]\n", COLOR_INFO);
    uint64_t start, current, end;
    vmm_get_heap_info(&start, &current, &end);

    if (vmm_is_initialized()) {
        puts_color("  Heap Info: SUCCESS\n", COLOR_SUCCESS);
        puts("    Start: 0x"); puts_hex64(start); puts("\n");
        puts("    Current: 0x"); puts_hex64(current); puts("\n");
        puts("    End: 0x"); puts_hex64(end); puts("\n");
        puts("    Total size: "); puts_uint64((end - start) / (1024 * 1024)); puts(" MB\n");
        puts("    Used: "); puts_uint64((current - start) / 1024); puts(" KB\n");
        puts("    Available: "); puts_uint64((end - current) / 1024); puts(" KB\n");
    } else {
        puts_color("  Heap Info: FAILED (VMM not initialized)\n", COLOR_ERROR);
    }
}

// Process management commands
static void cmd_ps(void *mbi) {
    puts_color("Process List:\n", COLOR_INFO);
    debug_process_table();
}

static void cmd_create_test(void *mbi) {
    puts_color("Creating test processes...\n", COLOR_INFO);
    create_test_processes();
    puts_color("Test processes created.\n", COLOR_SUCCESS);
}

static void cmd_start_scheduler(void *mbi) {
    puts_color("Starting scheduler...\n", COLOR_INFO);
    start_multitasking();
}

static void cmd_run_scheduler(void *mbi) {
    puts_color("Running scheduler once...\n", COLOR_INFO);
    run_scheduler();
}

static void cmd_run_process_0(void *mbi) {
    puts_color("Running process 0...\n", COLOR_INFO);
    run_process_by_pid(0);
}

static void cmd_run_process_1(void *mbi) {
    puts_color("Running process 1...\n", COLOR_INFO);
    run_process_by_pid(1);
}

static void cmd_multitask(void *mbi) {
    puts_color("Starting cooperative multitasking demo...\n", COLOR_INFO);
    
    // Run scheduler multiple times to see multitasking
    for (int i = 0; i < 30; i++) {
        run_scheduler();
        // Small delay to see what's happening
        for (volatile int j = 0; j < 500000; j++);
    }
    
    puts_color("\nMultitasking demo finished\n", COLOR_SUCCESS);
}

static void cmd_auto_multi_on(void *mbi) {
    enable_multitasking();
    puts_color("Automatic multitasking ENABLED\n", COLOR_SUCCESS);
    puts_color("Processes will switch automatically every 5 timer ticks\n", COLOR_INFO);
}

static void cmd_auto_multi_off(void *mbi) {
    disable_multitasking();
    puts_color("Automatic multitasking DISABLED\n", COLOR_WARNING);
}

// Update command registry
static shell_command_t commands[] = {
    {"test-all", "Run all tests", cmd_test_all},
    {"memory-info", "Show detailed memory information", cmd_memory_info},
    {"cat", "Show pretty cat", cmd_cat},
    {"random", "Show random number", cmd_random},
    {"ticks", "Show system ticks", cmd_ticks},
    {"memory", "Show memory information", cmd_memory},
    {"memmap", "Show physical memory map", cmd_memmap},
    {"ps", "Show process list", cmd_ps},
    {"create", "Create test processes", cmd_create_test},
    {"start", "Start multitasking", cmd_start_scheduler},
    {"sched", "Run scheduler once", cmd_run_scheduler},
    {"run0", "Run process 0", cmd_run_process_0},
    {"run1", "Run process 1", cmd_run_process_1},
    {"multi", "Cooperative multitasking demo", cmd_multitask},
    {"auto-on", "Enable automatic multitasking", cmd_auto_multi_on},
    {"auto-off", "Disable automatic multitasking", cmd_auto_multi_off},
    {"help", "Show this help", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {nullptr, nullptr, nullptr}
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
    
    puts_color("-*- Mini Shell -*-\n", COLOR_INFO);
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