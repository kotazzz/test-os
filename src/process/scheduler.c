#include "pcb.h"
#include "scheduler.h"
#include <stddef.h>
#include "vga.h"

#define MAX_PROCESSES 256

extern pcb_t process_table[MAX_PROCESSES];

static int current_index = -1;

pcb_t* schedule_next() {
    // puts("schedule_next: Looking for ready process, current_index=");
    // puts_hex64(current_index);
    // puts("\n");
    
    int start_index = current_index;
    int checked_count = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        current_index = (current_index + 1) % MAX_PROCESSES;
        checked_count++;
        
        // puts("schedule_next: Checking slot ");
        // puts_hex64(current_index);
        // puts(" state=");
        // puts_hex64(process_table[current_index].state);
        
        if (process_table[current_index].state == PROCESS_READY) {
            // puts(" -> FOUND!\n");
            // puts("schedule_next: Found ready process at ");
            // puts_hex64(current_index);
            // puts(" with entry point ");
            // puts_hex64(process_table[current_index].registers[15]);
            // puts("\n");
            return &process_table[current_index];
        } else {
            // puts(" -> skip\n");
        }
        
        // Prevent infinite loop
        if (checked_count >= MAX_PROCESSES) {
            break;
        }
    }
    
    puts("schedule_next: No ready processes found after checking ");
    puts_hex64(checked_count);
    puts(" slots\n");
    return NULL; // No ready processes
}

void run_scheduler() {
    puts("run_scheduler: Called\n");
    pcb_t *current = get_current_process();
    if (current) {
        puts("run_scheduler: Current process is ");
        puts_hex64(current->pid);
        puts("\n");
    } else {
        puts("run_scheduler: No current process\n");
    }
    
    pcb_t *next_process = schedule_next();
    
    if (next_process && next_process != current) {
        puts("run_scheduler: Switching from ");
        if (current) {
            puts_hex64(current->pid);
        } else {
            puts("NULL");
        }
        puts(" to ");
        puts_hex64(next_process->pid);
        puts("\n");
        
        switch_context(next_process);
    } else if (next_process == current) {
        puts("run_scheduler: Same process, no switch needed\n");
    } else {
        puts("run_scheduler: No next process found\n");
    }
}

void start_multitasking() {
    puts("start_multitasking: Looking for first process...\n");
    pcb_t *first_process = schedule_next();
    if (first_process) {
        puts("start_multitasking: Found process ");
        puts_hex64(first_process->pid);
        puts(", switching to it\n");
        switch_context(first_process);
    } else {
        puts("start_multitasking: No processes to start!\n");
    }
}
