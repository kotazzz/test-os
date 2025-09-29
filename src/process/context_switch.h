#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include "pcb.h"

// Assembly functions for context switching
extern void context_switch_irq_handler(void);
extern void save_current_context(void);
extern void restore_next_context(pcb_t *next_process);

// C helper functions for scheduler
pcb_t *schedule_next_process(void);
void init_context_switching(void);

// Global tick counter
extern unsigned long ticks;

#endif // CONTEXT_SWITCH_H