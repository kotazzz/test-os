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

// Обработчик таймера, когда многозадачность выключена
void timer_handler() {
    ticks++;
    outb(0x20, 0x20); // EOI (End of Interrupt)
}

void enable_multitasking() {
    multitasking_enabled = 1;
    init_context_switching(); // Эта функция теперь регистрирует правильный IRQ-хендлер
    puts("Preemptive multitasking enabled\n");
}

void disable_multitasking() {
    multitasking_enabled = 0;
    // Возвращаем старый обработчик
    REGISTER_IRQ_HANDLER(0, timer_handler);
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
    
    // По умолчанию регистрируем простой обработчик
    REGISTER_IRQ_HANDLER(0, timer_handler);
    puts("Timer initialized (legacy handler)\n");
}
