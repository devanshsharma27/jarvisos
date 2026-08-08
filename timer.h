#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define TIMER_HZ 100        // one tick every 10 ms

void     timer_init(void);
void     timer_tick(void);          // called from IRQ0
uint32_t timer_ticks(void);
uint32_t timer_seconds(void);
void     timer_sleep(uint32_t ms);  // busy-wait on the tick counter

#endif
