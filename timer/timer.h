#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint32_t tick;

void timer1_init(void);

#endif
