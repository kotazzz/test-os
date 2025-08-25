#include "pcb.h"
#include "vga.h"

// Process A as a state machine
void process_a() {
    pcb_t *current = get_current_process();
    if (!current) return;
    
    if (current->process_step == 0) {
        puts_color("[A] Started\n", COLOR_SUCCESS);
        current->process_counter = 0;
        current->process_step = 1;
    }
    
    if (current->process_step == 1) {
        // Main loop
        puts("[A] ");
        puts_hex64(current->process_counter);
        puts(" ");
        
        current->process_counter++;
        
        if (current->process_counter >= 10) {
            current->process_step = 2; // Go to finish
        } else if (current->process_counter % 2 == 0) {
            yield(); // Yield every 2nd iteration
        }
    }
    
    if (current->process_step == 2) {
        puts_color("[A] Done\n", COLOR_SUCCESS);
        current->state = PROCESS_TERMINATED;
    }
}

// Process B as a state machine  
void process_b() {
    pcb_t *current = get_current_process();
    if (!current) return;
    
    if (current->process_step == 0) {
        puts_color("[B] Started\n", COLOR_INFO);
        current->process_counter = 0;
        current->process_step = 1;
    }
    
    if (current->process_step == 1) {
        // Main loop
        puts("[B] ");
        puts_hex64(current->process_counter);
        puts(" ");
        
        current->process_counter++;
        
        if (current->process_counter >= 10) {
            current->process_step = 2; // Go to finish
        } else if (current->process_counter % 3 == 0) {
            yield(); // Yield every 3rd iteration
        }
    }
    
    if (current->process_step == 2) {
        puts_color("[B] Done\n", COLOR_INFO);
        current->state = PROCESS_TERMINATED;
    }
}

void create_test_processes() {
    pcb_t* proc_a = create_process(process_a);
    pcb_t* proc_b = create_process(process_b);
    
    if (proc_a && proc_b) {
        puts_color("Test processes created successfully\n", COLOR_SUCCESS);
    } else {
        puts_color("Failed to create test processes\n", COLOR_ERROR);
    }
}
