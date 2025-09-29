#include "context_switch.h"
#include "scheduler.h"
#include "pcb.h"
#include "../vga.h"

// Global tick counter (also defined in assembly)
unsigned long ticks = 0;

// Initialize context switching system
void init_context_switching(void) {
    // Register the assembly IRQ handler for timer interrupt
    extern void register_interrupt_handler(uint8_t n, void (*handler)(void));
    register_interrupt_handler(32, context_switch_irq_handler); // IRQ0 -> INT 32
    
    puts("Context switching system initialized\n");
}

// Schedule next process (called from assembly context_switch_irq_handler)
pcb_t *schedule_next_process(void) {
    // Use existing scheduler logic
    extern pcb_t *get_next_process(void);
    return get_next_process();
}

// Cooperative yield - allows process to voluntarily give up CPU
void yield_cooperative(void) {
    pcb_t *current = get_current_process();
    if (!current) {
        return;
    }

    // Mark current process as ready to run again
    if (current->state == PROCESS_RUNNING) {
        current->state = PROCESS_READY;
    }

    // Save current context and switch to next process
    save_current_context();
    
    // Get next process
    pcb_t *next = schedule_next_process();
    if (next && next != current) {
        set_current_process(next);
        next->state = PROCESS_RUNNING;
        restore_next_context(next);
    }
    
    // If we get here, we're back in the original process
    if (current) {
        current->state = PROCESS_RUNNING;
    }
}