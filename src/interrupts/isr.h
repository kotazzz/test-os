#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef void (*isr_t)(void);

void isr_handler(uint8_t n); // общий обработчик
void register_interrupt_handler(uint8_t n, isr_t handler);

extern void isr_stub_default(); // дефолтная заглушка
extern void irq0_handler();     // обработчик IRQ0 (таймер)

#endif
