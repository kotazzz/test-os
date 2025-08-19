#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Функции для ввода
char getc(void);
char* gets(char* buffer);

// Функции для работы с буфером клавиатуры
void keyboard_buffer_init(void);
void keyboard_buffer_put(char c);
char keyboard_buffer_get(void);
int keyboard_buffer_has_data(void);

#endif // INPUT_H