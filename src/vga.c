#include <stdint.h>
#include "vga.h"

int row=0, col=0;
uint8_t current_color = VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

volatile uint16_t* VGA = (uint16_t*)0xB8000;

void putc(char c){
    if(c == '\n'){ 
        col = 0; 
        row++; 
        if(row >= 25) {
            // Scroll screen up
            for(int i = 0; i < 80 * 24; i++) {
                VGA[i] = VGA[i + 80];
            }
            // Clear last line
            for(int i = 80 * 24; i < 80 * 25; i++) {
                VGA[i] = ' ' | (current_color << 8);
            }
            row = 24;
        }
        return; 
    }
    if(c == '\b'){
        if(col > 0) {
            col--;
            VGA[row * 80 + col] = ' ' | (current_color << 8);
        }
        return;
    }
    
    VGA[row * 80 + col] = (uint16_t)c | (current_color << 8);
    if(++col >= 80){ 
        col = 0; 
        row++; 
        if(row >= 25) {
            // Scroll screen up
            for(int i = 0; i < 80 * 24; i++) {
                VGA[i] = VGA[i + 80];
            }
            // Clear last line
            for(int i = 80 * 24; i < 80 * 25; i++) {
                VGA[i] = ' ' | (current_color << 8);
            }
            row = 24;
        }
    }
}

void puts(const char* s){
    while(*s) putc(*s++);
}

void puts_uint64(uint64_t num) {
    char buffer[21]; // Enough for 64-bit number in decimal + null terminator
    int i = 20;
    buffer[i--] = '\0'; // Null-terminate the string

    if (num == 0) {
        buffer[i--] = '0';
    } else {
        while (num > 0) {
            buffer[i--] = '0' + (num % 10);
            num /= 10;
        }
    }

    puts(&buffer[i + 1]); // Print the string starting from the first digit
}

void puts_hex64(uint64_t num) {
    char buffer[17]; // 16 hex digits + null terminator
    int i = 16;
    buffer[i--] = '\0';
    
    if (num == 0) {
        buffer[i--] = '0';
    } else {
        while (num > 0) {
            int digit = num & 0xF;
            if (digit < 10) {
                buffer[i--] = '0' + digit;
            } else {
                buffer[i--] = 'A' + (digit - 10);
            }
            num >>= 4;
        }
    }
    
    puts(&buffer[i + 1]);
}

// Color management functions
void set_text_color(uint8_t fg, uint8_t bg) {
    current_color = VGA_ENTRY_COLOR(fg, bg);
}

void set_text_color_fg(uint8_t fg) {
    current_color = VGA_ENTRY_COLOR(fg, current_color >> 4);
}

void set_text_color_bg(uint8_t bg) {
    current_color = VGA_ENTRY_COLOR(current_color & 0x0F, bg);
}

void putc_color(char c, uint8_t color) {
    uint8_t old_color = current_color;
    current_color = color;
    putc(c);
    current_color = old_color;
}

void puts_color(const char* s, uint8_t color) {
    uint8_t old_color = current_color;
    current_color = color;
    puts(s);
    current_color = old_color;
}

// Simplified functions with black background
void puts_fg(const char* s, uint8_t fg_color) {
    puts_color(s, VGA_ENTRY_COLOR(fg_color, VGA_COLOR_BLACK));
}

void puts_uint64_fg(uint64_t num, uint8_t fg_color) {
    uint8_t old_color = current_color;
    current_color = VGA_ENTRY_COLOR(fg_color, VGA_COLOR_BLACK);
    puts_uint64(num);
    current_color = old_color;
}

uint8_t get_current_color(void) {
    return current_color;
}
