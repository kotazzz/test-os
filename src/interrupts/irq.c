#include "irq.h"
#include "isr.h"
#include "isr_macros.h"
#include "../io.h"
#include "../vga.h"

void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    
    puts("Keyboard scancode: 0x");
    puts_hex64(scancode);
    puts("\n");
    
    outb(0x20, 0x20); // EOI
}

void timer_handler() {
    static int tick_count = 0;
    static int seconds = 0;
    tick_count++;
    if (tick_count >= 100) { // 100 тиков = 1 секунда
        tick_count = 0;
        seconds++;
        puts("Time passed: ");
        puts_uint64(seconds);
        puts(" seconds\n");
    }
    outb(0x20, 0x20); // EOI
}

void init_timer(int frequency) {
    int divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    REGISTER_IRQ_HANDLER(0, timer_handler);
}

void remap_pic() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void init_pic() {
    remap_pic();
    // маскируем все IRQ кроме нужных (таймер и клавиатура)
    outb(0x21, 0xFC); // IRQ0 (таймер) и IRQ1 (клавиатура) разрешены, остальные маскированы
    outb(0xA1, 0xFF); // маскируем все вторичные
}

void init_keyboard() {
    REGISTER_IRQ_HANDLER(1, keyboard_handler);
}