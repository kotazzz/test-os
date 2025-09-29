#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

// Function prototypes
void run_scheduler();
void start_multitasking(); // Start automatic scheduling
pcb_t* get_next_process(void); // Get next ready process for context switching

#endif // SCHEDULER_H
