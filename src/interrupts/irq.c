#include "irq.h"
#include "handlers/timer.h"
#include "handlers/pic.h"
#include "keyboard/keyboard_irq.h"

// Общая инициализация всех IRQ
void init_irq() {
    init_pic();
    init_keyboard_irq();
}