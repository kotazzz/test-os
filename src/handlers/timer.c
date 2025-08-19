#include "timer.h"
#include "../interrupts/isr_macros.h"
#include "../io.h"
#include "../vga.h"

unsigned long ticks = 0;

void timer_handler() {
    ticks++;
    outb(0x20, 0x20); // EOI
}

void init_timer(int frequency) {
    int divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    REGISTER_IRQ_HANDLER(0, timer_handler);
}
