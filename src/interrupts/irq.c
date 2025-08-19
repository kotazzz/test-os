#include "irq.h"
#include "isr.h"
#include "../io.h"
#include "../vga.h"

void timer_handler() {
    // Простой вывод для проверки - выводим точку каждые 100 тиков
    static int tick_count = 0;
    tick_count++;
    if (tick_count >= 500) {
        tick_count = 0;
    }
    puts_uint64(tick_count);
    puts("\n");
    outb(0x20, 0x20); // EOI (End of Interrupt)
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