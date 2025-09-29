#include "pcb.h"
#include "scheduler.h"
#include "../handlers/timer.h" // For disable_multitasking
#include "std/stddef.h"
#include "../vga.h"
#include "context_switch.h"

#define MAX_PROCESSES 256

extern pcb_t process_table[MAX_PROCESSES];

static int current_index = -1;

pcb_t* schedule_next() {
    int checked_count = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        current_index = (current_index + 1) % MAX_PROCESSES;
        checked_count++;
        
        if (process_table[current_index].state == PROCESS_READY) {
            return &process_table[current_index];
        }
        
        // Prevent infinite loop
        if (checked_count >= MAX_PROCESSES) {
            break;
        }
    }
    
    return NULL; // No ready processes
}

// Alias for the context switching system
pcb_t* get_next_process(void) {
    return schedule_next();
}

void run_scheduler() {
    pcb_t *current = get_current_process();
    
    // This function is for cooperative multitasking demo.
    // It should yield from the current context (shell) to a process.
    if (current) {
        // If we are already in a process, yield to the next one.
        yield();
    } else {
        // If we are in the shell (no current process), start the first one.
        // This will not return to the shell.
        start_multitasking();
    }
}

void start_multitasking() {
    pcb_t *first_process = schedule_next();
    if (first_process) {
        set_current_process(first_process);
        first_process->state = PROCESS_RUNNING;
        restore_next_context(first_process); // This will start the process and will not return here
    } else {
        puts_color("No processes to start.\n", COLOR_WARNING);
    }
}
