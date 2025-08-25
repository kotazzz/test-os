#ifndef TIMER_H
#define TIMER_H

extern unsigned long ticks;

// Инициализация таймера с заданной частотой
void init_timer(int frequency);

// Обработчик прерывания таймера
void timer_handler(void);

// Управление многозадачностью
void enable_multitasking();
void disable_multitasking();
void set_time_slice(int slice);

#endif
