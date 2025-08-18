#include <stdint.h>

static volatile uint16_t* VGA = (uint16_t*)0xB8000;
static int row=0, col=0;

static void putc(char c){
    if(c=='\n'){ col=0; row++; return; }
    VGA[row*80 + col] = (uint16_t)c | (0x07<<8);
    if(++col >= 80){ col=0; row++; }
}

static void puts(const char* s){
    while(*s) putc(*s++);
}

void kmain(void){
    row = 0;
    col = 0;
    for(int i=0;i<80*25;i++) VGA[i]=(' ' | (0x07<<8));
    puts("Hello from 64-bit kernel!\n");
    puts("Long mode successfully enabled!\n");
    for(;;) __asm__ volatile("hlt");
}
