#include "pcb.h"
#include "vga.h"

// Process A as a state machine
void process_a() {
    pcb_t *current = get_current_process();
    if (!current) return;
    
    if (current->process_step == 0) {
        puts_color("=== Process A Started ===\n", COLOR_SUCCESS);
        current->process_counter = 0;
        current->process_step = 1;
    }
    
    if (current->process_step == 1) {
        // Main loop
        puts("Process A: iteration ");
        puts_hex64(current->process_counter);
        puts("\n");
        
        current->process_counter++;
        
        if (current->process_counter >= 10) {
            current->process_step = 2; // Go to finish
        } else if (current->process_counter % 2 == 0) {
            puts("Process A: yielding...\n");
            yield(); // This should return here next time
        }
    }
    
    if (current->process_step == 2) {
        puts_color("=== Process A Finished ===\n", COLOR_SUCCESS);
        current->state = PROCESS_TERMINATED;
    }
}

// Process B as a state machine  
void process_b() {
    pcb_t *current = get_current_process();
    if (!current) return;
    
    if (current->process_step == 0) {
        puts_color("=== Process B Started ===\n", COLOR_INFO);
        current->process_counter = 0;
        current->process_step = 1;
    }
    
    if (current->process_step == 1) {
        // Main loop
        puts("Process B: iteration ");
        puts_hex64(current->process_counter);
        puts("\n");
        
        current->process_counter++;
        
        if (current->process_counter >= 10) {
            current->process_step = 2; // Go to finish
        } else if (current->process_counter % 3 == 0) {
            puts("Process B: yielding...\n");
            yield(); // This should return here next time
        }
    }
    
    if (current->process_step == 2) {
        puts_color("=== Process B Finished ===\n", COLOR_INFO);
        current->state = PROCESS_TERMINATED;
    }
}

void create_test_processes() {
    puts("create_test_processes: Creating process A...\n");
    pcb_t* proc_a = create_process(process_a);
    if (proc_a) {
        puts("create_test_processes: Process A created in slot ");
        puts_hex64(proc_a->pid);
        puts("\n");
    } else {
        puts("create_test_processes: Failed to create Process A\n");
    }
    
    puts("create_test_processes: Creating process B...\n");
    pcb_t* proc_b = create_process(process_b);
    if (proc_b) {
        puts("create_test_processes: Process B created in slot ");
        puts_hex64(proc_b->pid);
        puts("\n");
    } else {
        puts("create_test_processes: Failed to create Process B\n");
    }
    
    puts("create_test_processes: Done\n");
}
