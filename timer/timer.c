#include <avr/interrupt.h>
#include <avr/io.h>

#include "timer.h"

volatile uint32_t tick = 0;

// 初始化 timer1
void timer1_init(void)
{
    // CTC 模式
    TCCR1B |= (1 << WGM12);

    // 每 1 ms
    OCR1A = 249;

    // 开启 Compare 中断
    TIMSK1 |= (1 << OCIE1A);

    // 64 分频
    TCCR1B |= (1 << CS11) | (1 << CS10);
}

ISR(TIMER1_COMPA_vect)
{
    tick++;
}
