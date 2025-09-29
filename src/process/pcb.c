#include "pcb.h"
#include <stddef.h>
#include "../std/string.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"
#include "../vga.h"
#include "../gdt/gdt.h" // For user segment selectors

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
            
            // Clear the new fields
            p->page_directory = NULL;
            p->page_directory_phys = 0;
            p->user_stack_virt = 0;
            p->user_stack_phys = 0;

            if (is_user) {
                // Create separate address space for user process
                uint64_t *new_pml4 = vmm_create_address_space();
                if (!new_pml4) {
                    puts("Error: Failed to create address space for user process\n");
                    return NULL;
                }
                
                p->page_directory = new_pml4;
                p->page_directory_phys = (uint64_t)new_pml4;
                
                // Allocate user stack in physical memory
                uint64_t user_stack_phys = pmm_alloc_page();
                if (user_stack_phys == 0) {
                    puts("Error: Failed to allocate user stack page\n");
                    vmm_destroy_address_space(new_pml4);
                    return NULL;
                }
                p->user_stack_phys = user_stack_phys;
                
                // Map user stack to user virtual address space
                p->user_stack_virt = USER_VIRTUAL_BASE + 0x10000; // User stack at 4MB + 64KB
                if (!vmm_map_user_page(new_pml4, p->user_stack_virt, user_stack_phys)) {
                    puts("Error: Failed to map user stack\n");
                    pmm_free_page(user_stack_phys);
                    vmm_destroy_address_space(new_pml4);
                    return NULL;
                }
                
                // Allocate kernel stack for interrupt handling
                uint64_t kernel_stack_page = pmm_alloc_page();
                if (kernel_stack_page == 0) {
                    puts("Error: Failed to allocate kernel stack page\n");
                    pmm_free_page(user_stack_phys);
                    vmm_destroy_address_space(new_pml4);
                    return NULL;
                }
                p->stack_base = kernel_stack_page;
                p->stack_pointer = (uint64_t*)(kernel_stack_page + STACK_SIZE);
                
                // Create full interrupt context on kernel stack for context switching
                uint64_t *stack = p->stack_pointer;
                uint64_t user_stack_top = p->user_stack_virt + PAGE_SIZE - 16; // Top of user stack, aligned
                
                puts("Setting up user process context for context switching:\n");
                puts("  Entry point: ");
                puts_hex64((uint64_t)entry_point);
                puts("\n  PML4 phys: ");
                puts_hex64((uint64_t)new_pml4);
                puts("\n  User stack virt: ");
                puts_hex64(p->user_stack_virt);
                puts("\n  User stack top: ");
                puts_hex64(user_stack_top);
                puts("\n");
                
                // Set up interrupt frame (what CPU pushes during interrupt)
                *--stack = USER_DATA_SEGMENT;    // SS
                *--stack = user_stack_top;       // RSP 
                *--stack = 0x202;                // RFLAGS (IF=1)
                *--stack = USER_CODE_SEGMENT;    // CS
                *--stack = (uint64_t)entry_point;// RIP
                
                // Set up register context (what our context switcher saves/restores)
                *--stack = 0;                    // RBP
                *--stack = 0;                    // RAX
                *--stack = 0;                    // RBX
                *--stack = 0;                    // RCX
                *--stack = 0;                    // RDX
                *--stack = 0;                    // RSI
                *--stack = 0;                    // RDI
                *--stack = 0;                    // R8
                *--stack = 0;                    // R9
                *--stack = 0;                    // R10
                *--stack = 0;                    // R11
                *--stack = 0;                    // R12
                *--stack = 0;                    // R13
                *--stack = 0;                    // R14
                *--stack = 0;                    // R15
                
                p->stack_pointer = stack; // Point to saved context
                
            } else {
                // Kernel process - simpler setup
                uint64_t stack_page = pmm_alloc_page();
                if (stack_page == 0) {
                    puts("Error: Failed to allocate stack page\n");
                    return NULL;
                }
                
                p->stack_base = stack_page;
                p->stack_pointer = (uint64_t*)(stack_page + STACK_SIZE);
                p->page_directory = (uint64_t*)vmm_get_current_page_directory();
                p->page_directory_phys = vmm_get_current_page_directory();
                
                // Create kernel interrupt context
                uint64_t *stack = p->stack_pointer;
                
                // Set up interrupt frame for kernel process
                *--stack = 0x10;                 // SS (kernel data)
                uint64_t new_rsp = (uint64_t)stack - 40;
                *--stack = new_rsp;              // RSP (just below interrupt frame)
                *--stack = 0x202;                // RFLAGS (IF=1)
                *--stack = 0x08;                 // CS (kernel code)
                *--stack = (uint64_t)entry_point;// RIP
                
                // Set up register context  
                *--stack = 0;                    // RBP
                *--stack = 0;                    // RAX
                *--stack = 0;                    // RBX
                *--stack = 0;                    // RCX
                *--stack = 0;                    // RDX
                *--stack = 0;                    // RSI
                *--stack = 0;                    // RDI
                *--stack = 0;                    // R8
                *--stack = 0;                    // R9
                *--stack = 0;                    // R10
                *--stack = 0;                    // R11
                *--stack = 0;                    // R12
                *--stack = 0;                    // R13
                *--stack = 0;                    // R14
                *--stack = 0;                    // R15
                
                p->stack_pointer = stack; // Point to saved context
            }
            
            // Initialize registers to 0
            memset(p->registers, 0, sizeof(p->registers));
            
            // Store entry point
            p->registers[15] = (uint64_t)entry_point;
            
            return p;
        }
    }
    
    return NULL; // No available slots
}

void terminate_process(uint32_t pid) {
    if (pid < MAX_PROCESSES && process_table[pid].state != PROCESS_TERMINATED) {
        pcb_t *p = &process_table[pid];
        
        // Free the allocated stack using saved base address
        if (p->stack_base) {
            pmm_free_page(p->stack_base);
        }
        
        // For user processes, free their address space and user stack
        if (p->is_user_process && p->page_directory) {
            if (p->user_stack_phys) {
                pmm_free_page(p->user_stack_phys);
            }
            vmm_destroy_address_space(p->page_directory);
        }
        
        p->state = PROCESS_TERMINATED;
        p->stack_pointer = NULL;
        p->stack_base = 0;
        p->page_directory = NULL;
        p->page_directory_phys = 0;
        p->user_stack_virt = 0;
        p->user_stack_phys = 0;
        
        // If terminating current process, clear reference
        if (current_process == p) {
            current_process = NULL;
        }
    }
}

pcb_t* get_current_process() {
    return current_process;
}

void set_current_process(pcb_t* process) {
    current_process = process;
}



void yield() {
    // Use the new cooperative yield function
    extern void yield_cooperative(void);
    yield_cooperative();
}

void debug_process_table() {
    puts("=== Process Table ===\n");
    puts("PCB structure offsets (for assembly):\n");
    puts("  stack_pointer: "); puts_hex64(offsetof(pcb_t, stack_pointer)); puts("\n");
    puts("  page_directory_phys: "); puts_hex64(offsetof(pcb_t, page_directory_phys)); puts("\n");
    puts("  is_user_process: "); puts_hex64(offsetof(pcb_t, is_user_process)); puts("\n");
    puts("\n");
    
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
            puts(" KStack: ");
            puts_hex64((uint64_t)process_table[i].stack_pointer);
            puts(" Entry: ");
            puts_hex64(process_table[i].registers[15]);
            puts(" User: ");
            puts_hex64(process_table[i].is_user_process);
            if (process_table[i].is_user_process) {
                puts(" PML4: ");
                puts_hex64(process_table[i].page_directory_phys);
                puts(" UStack: ");
                puts_hex64(process_table[i].user_stack_virt);
            }
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
    if (pid >= MAX_PROCESSES || (process_table[pid].state != PROCESS_READY && process_table[pid].state != PROCESS_UNINITIALIZED)) {
        return;
    }
    
    pcb_t *proc = &process_table[pid];
    
    set_current_process(proc);
    proc->state = PROCESS_RUNNING;

    extern void restore_next_context(pcb_t*);
    restore_next_context(proc); // This will start the process and not return
}

pcb_t* create_user_process(void (*entry_point)()) {
    return create_process(entry_point, 1);
}

pcb_t* create_kernel_process(void (*entry_point)()) {
    return create_process(entry_point, 0);
}
