#include "pic.h"
#include "../io.h"

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
