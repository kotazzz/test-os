#include "scancode.h"
#include "../vga.h"

static const char scancode_to_ascii[128] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',
    '9',  '0',  '-',  '=',  '\b', '\t', 'q',  'w',  'e',  'r',
    't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n', 0,
    'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',  'b',  'n',
    'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0
};

static const char scancode_to_ascii_shift[128] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',
    '(',  ')',  '_',  '+',  '\b', '\t', 'Q',  'W',  'E',  'R',
    'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\n', 0,
    'A',  'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',  'B',  'N',
    'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0
};

static uint8_t shift_pressed = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t caps_lock = 0;

char process_scancode(uint8_t scancode) {
    // Проверяем отпускание клавиши
    if (scancode & 0x80) {
        scancode &= 0x7F; // убираем бит отпускания
        
        // Обрабатываем отпускание модификаторов
        switch (scancode) {
            case 0x2A: // Left Shift
            case 0x36: // Right Shift
                shift_pressed = 0;
                break;
            case 0x1D: // Ctrl
                ctrl_pressed = 0;
                break;
            case 0x38: // Alt
                alt_pressed = 0;
                break;
        }
        return 0; // не возвращаем символ при отпускании
    }
    
    // Обрабатываем нажатие модификаторов
    switch (scancode) {
        case 0x2A: // Left Shift
        case 0x36: // Right Shift
            shift_pressed = 1;
            return 0;
        case 0x1D: // Ctrl
            ctrl_pressed = 1;
            return 0;
        case 0x38: // Alt
            alt_pressed = 1;
            return 0;
        case 0x3A: // Caps Lock
            caps_lock = !caps_lock;
            return 0;
    }
    
    // Получаем ASCII символ
    char ascii = 0;
    if (scancode < 128) {
        if (shift_pressed) {
            ascii = scancode_to_ascii_shift[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
        }
        
        // Обрабатываем Caps Lock для букв
        if (caps_lock && ascii >= 'a' && ascii <= 'z') {
            ascii = ascii - 'a' + 'A';
        } else if (caps_lock && ascii >= 'A' && ascii <= 'Z') {
            ascii = ascii - 'A' + 'a';
        }
    }
    
    return ascii;
}

uint8_t is_shift_pressed() {
    return shift_pressed;
}

uint8_t is_ctrl_pressed() {
    return ctrl_pressed;
}

uint8_t is_alt_pressed() {
    return alt_pressed;
}

uint8_t is_caps_lock_on() {
    return caps_lock;
}