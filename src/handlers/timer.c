#include "timer.h"
#include "interrupts/isr_macros.h"
#include "interrupts/isr.h"
#include "io.h"
#include "vga.h"
#include "../process/context_switch.h"

// Note: ticks is now defined in context_switch.c and context_switch.s
extern unsigned long ticks;

static int multitasking_enabled = 0;
static int time_slice = 5; // Switch every 5 ticks

// Legacy timer handler - now only used for non-multitasking timer events
void timer_handler() {
    ticks++;
    
    // This handler is no longer used for context switching
    // Context switching is now handled by context_switch_irq_handler in assembly
    
    outb(0x20, 0x20); // EOI (End of Interrupt)
}

void enable_multitasking() {
    multitasking_enabled = 1;
    
    // Initialize the new context switching system
    init_context_switching();
    
    puts("Preemptive multitasking enabled\n");
}

void disable_multitasking() {
    multitasking_enabled = 0;
    puts("Multitasking disabled\n");
}

void set_time_slice(int slice) {
    if (slice > 0) {
        time_slice = slice;
    }
}

void init_timer(int frequency) {
    puts("Setting up timer at ");
    puts_hex64(frequency);
    puts(" Hz\n");
    
    int divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    // Use the legacy handler for now - multitasking will replace it
    REGISTER_IRQ_HANDLER(0, timer_handler);
    puts("Timer initialized (legacy handler)\n");
}
