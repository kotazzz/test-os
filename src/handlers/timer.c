#include "timer.h"
#include "interrupts/isr_macros.h"
#include "io.h"
#include "vga.h"

unsigned long ticks = 0;
static int multitasking_enabled = 0;
static int time_slice = 5; // Switch every 5 ticks

void timer_handler() {
    ticks++;
    
    // Automatic process switching every time_slice ticks
    if (multitasking_enabled && (ticks % time_slice == 0)) {
        extern void run_scheduler();
        run_scheduler();
    }
    
    outb(0x20, 0x20); // EOI (End of Interrupt)
}

void enable_multitasking() {
    multitasking_enabled = 1;
}

void disable_multitasking() {
    multitasking_enabled = 0;
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

    REGISTER_IRQ_HANDLER(0, timer_handler);
    puts("Timer initialized\n");
}
