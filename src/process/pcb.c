#include "pcb.h"
#include <stddef.h>
#include "std/string.h"
#include "memory/vmm.h"
#include "memory/pmm.h"
#include "vga.h"
#include "gdt/gdt.h" // For user segment selectors

#define MAX_PROCESSES 256
#define STACK_SIZE 4096  // 4KB stack per process

pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_process = NULL;

void init_process_system() {
    memset(process_table, 0, sizeof(process_table));
    current_process = NULL;
}

pcb_t* create_process(void (*entry_point)(), int is_user) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNINITIALIZED || process_table[i].state == PROCESS_TERMINATED) {
            pcb_t *p = &process_table[i];

            // Allocate stack
            uint64_t stack_page = pmm_alloc_page();
            if (stack_page == 0) {
                puts("Error: Failed to allocate stack page\n");
                return NULL; // Failed to allocate memory
            }

            if (entry_point == NULL) {
                puts("Error: Invalid entry point\n");
                return NULL; // Invalid entry point
            }

            // Initialize PCB
            p->pid = i;
            p->state = PROCESS_READY;
            p->is_user_process = is_user;
            p->process_step = 0;
            p->process_counter = 0;
            p->stack_pointer = (uint64_t*)(stack_page + STACK_SIZE);

            // Setup stack for context switch
            uint64_t *stack = p->stack_pointer;
            
            if (is_user) {
                // For user processes, we need to set up for iretq
                // Stack frame for iretq (pushed in reverse order):
                // SS (Stack Segment), RSP (Stack Pointer), RFLAGS, CS (Code Segment), RIP (Instruction Pointer)
                
                // Create a separate user stack - allocate another page for user space
                uint64_t user_stack_page = pmm_alloc_page();
                if (user_stack_page == 0) {
                    puts("Error: Failed to allocate user stack page\n");
                    return NULL;
                }
                uint64_t user_stack_top = user_stack_page + STACK_SIZE - 16; // Leave space and align to 16 bytes
                user_stack_top &= ~0xF; // Ensure 16-byte alignment
                
                puts("Setting up user process:\n");
                puts("  Entry point: ");
                puts_hex64((uint64_t)entry_point);
                puts("\n  Kernel stack base: ");
                puts_hex64((uint64_t)p->stack_pointer);
                puts("\n  User stack top (aligned): ");
                puts_hex64(user_stack_top);
                puts("\n");
                
                // Set up iretq frame on kernel stack (stack points to kernel stack)
                *--stack = USER_DATA_SEGMENT;    // SS - User data segment (0x23)
                *--stack = user_stack_top;       // RSP - User stack pointer 
                *--stack = 0x202;                // RFLAGS - IF=1, IOPL=0 for user mode
                *--stack = USER_CODE_SEGMENT;    // CS - User code segment (0x1B)
                *--stack = (uint64_t)entry_point;// RIP - Entry point address
                
                puts("  iretq frame at: ");
                puts_hex64((uint64_t)stack);
                puts("\n  Frame contents:\n");
                puts("    RIP: ");
                puts_hex64(stack[0]);
                puts("\n    CS:  ");
                puts_hex64(stack[1]);
                puts("\n    RFLAGS: ");
                puts_hex64(stack[2]);
                puts("\n    RSP: ");
                puts_hex64(stack[3]);
                puts("\n    SS:  ");
                puts_hex64(stack[4]);
                puts("\n");
                
            } else {
                // For kernel processes, just store entry point for direct call
                // No stack frame needed - we'll call the function directly
            }
            
            // Initialize registers to 0
            memset(p->registers, 0, sizeof(p->registers));
            
            if (is_user) {
                // For user processes, save the prepared stack with iret frame
                p->registers[7] = (uint64_t)stack; // RSP pointing to iret frame
                p->stack_pointer = stack; // Update stack_pointer to point to iret frame
            } else {
                // For kernel processes
                p->registers[7] = (uint64_t)stack; // RSP
            }
            p->registers[15] = (uint64_t)entry_point; // Store entry point in register 15
            
            return p;
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
        return;
    }
    
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
    }

    current_process = next_process;
    current_process->state = PROCESS_RUNNING;

    if (next_process->is_user_process) {
        // Switch to user mode using iretq
        // The user process stack is already set up with proper segments and entry point
        
        puts_color("[KERNEL] Switching to user mode process PID ", COLOR_INFO);
        puts_hex64(next_process->pid);
        puts("\n[KERNEL] User stack pointer: ");
        puts_hex64((uint64_t)next_process->stack_pointer);
        puts("\n[KERNEL] About to execute iretq...\n");
        
        // Verify stack is properly aligned and within bounds
        uint64_t stack_addr = (uint64_t)next_process->stack_pointer;
        if (stack_addr == 0 || (stack_addr & 0x7) != 0) {
            puts_color("ERROR: Invalid stack pointer for user process!\n", COLOR_ERROR);
            next_process->state = PROCESS_TERMINATED;
            current_process = NULL;
            return;
        }
        
        // Debug: Print the entire iretq frame
        uint64_t *debug_stack = next_process->stack_pointer;
        puts("iretq frame contents:\n");
        puts("  [0] RIP: ");
        puts_hex64(debug_stack[0]);
        puts("\n  [1] CS:  ");
        puts_hex64(debug_stack[1]);  
        puts("\n  [2] RFLAGS: ");
        puts_hex64(debug_stack[2]);
        puts("\n  [3] RSP: ");
        puts_hex64(debug_stack[3]);
        puts("\n  [4] SS:  ");
        puts_hex64(debug_stack[4]);
        puts("\n");
        
        // TEMPORARY: Skip iretq and just call the function directly to test
        puts_color("TEMP: Calling user function directly instead of iretq\n", COLOR_WARNING);
        void (*entry_point)() = (void(*)())debug_stack[0];
        if (entry_point) {
            puts_color("TEMP: About to call user function\n", COLOR_INFO);
            entry_point();
            puts_color("TEMP: User function returned successfully\n", COLOR_SUCCESS);
        } else {
            puts_color("TEMP: No entry point found\n", COLOR_ERROR);
        }
        
        // Terminate the process after execution
        puts_color("TEMP: Terminating process after execution\n", COLOR_INFO);
        next_process->state = PROCESS_TERMINATED;
        current_process = NULL;
        return;
        
        // Load user process stack and switch to Ring 3
        __asm__ volatile (
            // Set up user data segments before switching
            "mov $0x23, %%ax\n"        // User data segment (Ring 3)
            "mov %%ax, %%ds\n"
            "mov %%ax, %%es\n"
            "mov %%ax, %%fs\n"
            "mov %%ax, %%gs\n"
            
            // Switch to user stack and mode using the prepared iret frame
            "mov %0, %%rsp\n"          // Load stack pointer to iret frame
            "iretq\n"                  // Return to user mode (Ring 3)
            :
            : "r"(next_process->stack_pointer)  // Use stack_pointer, not registers[7]
            : "memory"
        );
        
        // This code should never be reached - user mode will use syscalls to return
        puts_color("ERROR: Returned from user mode unexpectedly!\n", COLOR_ERROR);
        next_process->state = PROCESS_TERMINATED;
        current_process = NULL;
        
    } else {
        // For kernel processes - call the entry point directly
        void (*entry_point)() = (void(*)())next_process->registers[15];
        if (entry_point) {
            entry_point();
        } else {
            puts_color("ERROR: No entry point for kernel process\n", COLOR_ERROR);
            next_process->state = PROCESS_TERMINATED;
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
            puts(" User: ");
            puts_hex64(process_table[i].is_user_process);
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

pcb_t* create_user_process(void (*entry_point)()) {
    return create_process(entry_point, 1);
}

pcb_t* create_kernel_process(void (*entry_point)()) {
    return create_process(entry_point, 0);
}
