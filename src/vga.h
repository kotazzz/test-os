#ifndef VGA_H
#define VGA_H

#include <stdint.h>

// VGA color constants
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_WHITE         15

// Helper macro to create color attribute
#define VGA_ENTRY_COLOR(fg, bg) ((fg) | (bg) << 4)

// Shorthand macros for common colors with black background
#define COLOR_INFO      VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK)
#define COLOR_SUCCESS   VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK)
#define COLOR_WARNING   VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK)
#define COLOR_ERROR     VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK)
#define COLOR_TEXT      VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)
#define COLOR_WHITE     VGA_ENTRY_COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLACK)

extern int row, col;
extern volatile uint16_t* VGA;
extern uint8_t current_color;

void puts(const char* s);
void putc(char c);
void puts_uint64(uint64_t num);
void puts_hex64(uint64_t num);

// Simplified color functions
void puts_fg(const char* s, uint8_t fg_color);  // Black background by default
void puts_uint64_fg(uint64_t num, uint8_t fg_color);  // Colored number output

// Color functions
void set_text_color(uint8_t fg, uint8_t bg);
void set_text_color_fg(uint8_t fg);
void set_text_color_bg(uint8_t bg);
void putc_color(char c, uint8_t color);
void puts_color(const char* s, uint8_t color);
uint8_t get_current_color(void);

#endif // VGA_H