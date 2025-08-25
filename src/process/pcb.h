#ifndef PCB_H
#define PCB_H

#include <stdint.h>

// Process states
typedef enum {
    PROCESS_UNINITIALIZED = 0,  // Process slot is empty
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

// Process Control Block (PCB)
typedef struct {
    uint32_t pid;                // Process ID
    process_state_t state;       // Current state of the process
    uint64_t *stack_pointer;     // Pointer to the process's stack (64-bit)
    uint64_t *page_directory;    // Pointer to the process's page directory (CR3) (64-bit)
    uint64_t registers[16];      // General-purpose registers (RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15)
    
    // For cooperative multitasking - process state
    uint32_t process_counter;    // Internal counter for process logic
    uint32_t process_step;       // Current step in process execution
} pcb_t;

// Function prototypes
void init_process_system();
pcb_t* create_process(void (*entry_point)());
void terminate_process(uint32_t pid);
void switch_context(pcb_t *next_process);
pcb_t* get_current_process();
void yield(); // Allow process to voluntarily give up CPU
void debug_process_table(); // Debug function to show process states
void run_process_by_pid(uint32_t pid); // Run specific process by PID

#endif // PCB_H
