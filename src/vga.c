#include <stdint.h>
int row=0, col=0;

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
                VGA[i] = ' ' | (0x07 << 8);
            }
            row = 24;
        }
        return; 
    }
    if(c == '\b'){
        if(col > 0) {
            col--;
            VGA[row * 80 + col] = ' ' | (0x07 << 8);
        }
        return;
    }
    
    VGA[row * 80 + col] = (uint16_t)c | (0x07 << 8);
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
                VGA[i] = ' ' | (0x07 << 8);
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
