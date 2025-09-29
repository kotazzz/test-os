#include "pcb.h"
#include "scheduler.h"
#include "../handlers/timer.h" // For disable_multitasking
#include "std/stddef.h"
#include "../vga.h"

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
    pcb_t *next_process = schedule_next();
    
    if (next_process && next_process != current) {
        switch_context(next_process);
    } else if (next_process == current) {
        // Same process, call it again for state machine continuation
        switch_context(next_process);
    } else {
        // No ready processes - disable automatic multitasking to prevent infinite calls
        puts_color("No ready processes found - disabling automatic multitasking\n", COLOR_WARNING);
        extern void disable_multitasking();
        disable_multitasking();
    }
}
void start_multitasking() {
    pcb_t *first_process = schedule_next();
    if (first_process) {
        switch_context(first_process);
    }
}
