#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

void remap_pic();
void init_timer(int frequency);
void init_pic();

#endif
