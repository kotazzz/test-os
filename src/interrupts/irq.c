#include "irq.h"
#include "isr.h"
#include "../io.h"
#include "../vga.h"

void format_time(int seconds) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    puts("Passed time: ");
    if (hours < 10) puts("0");
    puts_uint64(hours);
    puts(":");
    if (minutes < 10) puts("0");
    puts_uint64(minutes);
    puts(":");
    if (seconds < 10) puts("0");
    puts_uint64(seconds);
    puts("\n");
    row--;
}

void timer_handler() {
    static int tick_count = 0;
    static int seconds = 0;
    tick_count++;
    if (tick_count >= 100) { // 100 тиков = 1 секунда
        tick_count = 0;
        seconds++;
        format_time(seconds);
    }
    outb(0x20, 0x20); // EOI
}


void init_timer(int frequency) {
    int divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    register_interrupt_handler(32, timer_handler);
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
    // маскируем все IRQ кроме нужных (например таймер)
    outb(0x21, 0xFE); // IRQ0 (таймер) разрешен, остальные маскированы
    outb(0xA1, 0xFF); // маскируем все вторичные
}