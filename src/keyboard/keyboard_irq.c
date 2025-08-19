#include "keyboard_irq.h"
#include "scancode.h"
#include "input.h"
#include "interrupts/isr_macros.h"
#include "io.h"
#include "vga.h"

void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    
    // Обрабатываем scancode и получаем символ
    char c = process_scancode(scancode);
    
    // Если получили валидный символ, добавляем его в буфер
    if (c != 0) {
        keyboard_buffer_put(c);
        
        // Выводим символ на экран (эхо) только если это не backspace
        if (c != '\b') {
            char str[2] = {c, '\0'};
            puts(str);
        }
    }
    
    outb(0x20, 0x20); // EOI
}

void init_keyboard_irq() {
    keyboard_buffer_init(); // Инициализируем буфер клавиатуры
    REGISTER_IRQ_HANDLER(1, keyboard_handler);
}
