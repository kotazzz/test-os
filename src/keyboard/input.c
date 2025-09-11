#include "scancode.h"
#include "input.h"
#include "std/stddef.h"

// Кольцевой буфер для клавиатуры
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0;

void keyboard_buffer_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
}

void keyboard_buffer_put(char c) {
    if (c == 0) return; // не добавляем нулевые символы
    
    int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head != buffer_tail) { // проверяем, что буфер не полный
        keyboard_buffer[buffer_head] = c;
        buffer_head = next_head;
    }
}

char keyboard_buffer_get(void) {
    if (buffer_head == buffer_tail) {
        return 0; // буфер пуст
    }
    
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

int keyboard_buffer_has_data(void) {
    return buffer_head != buffer_tail;
}

char getc(void) {
    char c = 0;
    while (c == 0) {
        // Ждем символ из буфера
        c = keyboard_buffer_get();
    }
    return c;
}

char* gets(char* buffer) {
    if (buffer == NULL) {
        return NULL;
    }
    
    char* start = buffer;
    char c;
    
    while ((c = getc()) != '\n' && c != '\r') {
        if (c == '\b') { // Backspace
            if (buffer > start) {
                buffer--;
                // Визуально стираем символ: backspace, пробел, backspace
                puts("\b \b");
            }
        } else {
            *buffer++ = c;
            // Символ уже выведен в keyboard_handler, поэтому не дублируем
        }
    }
    
    *buffer = '\0';
    return start;
}