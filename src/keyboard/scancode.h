#ifndef SCANCODE_H
#define SCANCODE_H

#include <stdint.h>

char process_scancode(uint8_t scancode);
uint8_t is_shift_pressed();
uint8_t is_ctrl_pressed();
uint8_t is_alt_pressed();
uint8_t is_caps_lock_on();

#endif // SCANCODE_H