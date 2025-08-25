#include "pcb.h"
#include <stddef.h>
#include "std/string.h"
#include "memory/vmm.h"
#include "memory/pmm.h"

#define MAX_PROCESSES 256
#define STACK_SIZE 4096  // 4KB stack per process

pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_process = NULL;

void init_process_system() {
    memset(process_table, 0, sizeof(process_table));
    current_process = NULL;
}

pcb_t* create_process(void (*entry_point)()) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        // Look for completely empty slots (state = UNINITIALIZED) or terminated processes
        if (process_table[i].state == PROCESS_UNINITIALIZED || process_table[i].state == PROCESS_TERMINATED) {
            // Allocate stack for the process
            uint64_t stack_page = pmm_alloc_page();
            if (stack_page == 0) {
                return NULL; // Failed to allocate memory
            }
            
            process_table[i].pid = i;
            process_table[i].state = PROCESS_READY;  // Set to READY (1)
            
            // Set up stack - stack grows downward, so point to end
            process_table[i].stack_pointer = (uint64_t*)(stack_page + STACK_SIZE - sizeof(uint64_t));
            process_table[i].page_directory = NULL; // TODO: Set up page directory here
            
            // Store entry point for simple switching
            memset(process_table[i].registers, 0, sizeof(process_table[i].registers));
            process_table[i].registers[15] = (uint64_t)entry_point; // Store entry point
            
            // Initialize process state machine
            process_table[i].process_counter = 0;
            process_table[i].process_step = 0;
            
            return &process_table[i];
        }
    }
    
    return NULL; // No available slots
}

void terminate_process(uint32_t pid) {
    if (pid < MAX_PROCESSES && process_table[pid].state != PROCESS_TERMINATED) {
        // Free the allocated stack
        if (process_table[pid].stack_pointer) {
            // Calculate original stack base from current stack pointer
            uint64_t stack_base = ((uint64_t)process_table[pid].stack_pointer) & ~(STACK_SIZE - 1);
            pmm_free_page(stack_base);
        }
        
        process_table[pid].state = PROCESS_TERMINATED;
        process_table[pid].stack_pointer = NULL;
        
        // If terminating current process, clear reference
        if (current_process == &process_table[pid]) {
            current_process = NULL;
        }
    }
}

pcb_t* get_current_process() {
    return current_process;
}

void switch_context(pcb_t *next_process) {
    if (!next_process || next_process->state == PROCESS_TERMINATED) {
        return; // Invalid process
    }
    
    if (current_process != next_process) {
        // Save state of current process
        if (current_process && current_process->state == PROCESS_RUNNING) {
            current_process->state = PROCESS_READY;
        }

        current_process = next_process;
        current_process->state = PROCESS_RUNNING;
        
        // Call process function (it should handle its own state)
        if (next_process->registers[15]) { // Entry point stored here
            void (*entry_point)() = (void(*)())next_process->registers[15];
            entry_point(); // Process will handle its own state and may yield
        }
    } else {
        // Even if same process, call it again (for state machine)
        if (next_process->registers[15] && next_process->state == PROCESS_RUNNING) {
            void (*entry_point)() = (void(*)())next_process->registers[15];
            entry_point();
        }
    }
}

void yield() {
    // Allow current process to voluntarily give up CPU
    extern void run_scheduler();
    run_scheduler();
}

void debug_process_table() {
    puts("=== Process Table ===\n");
    int process_count = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_UNINITIALIZED) {  // Show all initialized processes
            process_count++;
            puts("PID: ");
            puts_hex64(i);
            puts(" State: ");
            switch(process_table[i].state) {
                case PROCESS_UNINITIALIZED: puts("UNINITIALIZED"); break;
                case PROCESS_READY: puts("READY"); break;
                case PROCESS_RUNNING: puts("RUNNING"); break;
                case PROCESS_WAITING: puts("WAITING"); break;
                case PROCESS_TERMINATED: puts("TERMINATED"); break;
                default: puts("UNKNOWN("); puts_hex64(process_table[i].state); puts(")"); break;
            }
            puts(" Stack: ");
            puts_hex64((uint64_t)process_table[i].stack_pointer);
            puts(" Entry: ");
            puts_hex64(process_table[i].registers[15]);
            puts(" Step: ");
            puts_hex64(process_table[i].process_step);
            puts(" Counter: ");
            puts_hex64(process_table[i].process_counter);
            puts("\n");
        }
    }
    
    if (process_count == 0) {
        puts("No processes found\n");
    }
    
    puts("Current: ");
    if (current_process) {
        puts_hex64(current_process->pid);
    } else {
        puts("NULL");
    }
    puts("\n==================\n");
}

void run_process_by_pid(uint32_t pid) {
    if (pid >= MAX_PROCESSES || process_table[pid].state != PROCESS_READY) {
        return;
    }
    
    switch_context(&process_table[pid]);
}
