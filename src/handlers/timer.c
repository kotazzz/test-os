#include "timer.h"
#include "interrupts/isr_macros.h"
#include "io.h"
#include "vga.h"

unsigned long ticks = 0;

void timer_handler() {
    ticks++;
    // Simple debug - show timer is working (every 50 ticks to avoid spam)
    // if (ticks % 50 == 0) {
    //     puts("T");
    // }
    outb(0x20, 0x20); // EOI (End of Interrupt)
}

void init_timer(int frequency) {
    puts("Setting up timer with frequency: ");
    puts_hex64(frequency);
    puts(" Hz\n");
    
    int divisor = 1193180 / frequency;
    puts("Timer divisor: ");
    puts_hex64(divisor);
    puts("\n");
    
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    puts("Registering timer handler for IRQ 0 (interrupt 32)\n");
    REGISTER_IRQ_HANDLER(0, timer_handler);
    puts("Timer initialization complete\n");
}
