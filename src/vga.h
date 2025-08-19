#ifndef VGA_H
#define VGA_H

#include <stdint.h>

extern int row, col;
extern volatile uint16_t* VGA;

void puts(const char* s);
void putc(char c);
void puts_uint64(uint64_t num);
void puts_hex64(uint64_t num);

#endif // VGA_H