#include <avr/interrupt.h>
#include <avr/io.h>

#include "../task/task.h"
#include "timer.h"

volatile uint32_t tick = 0;

// 初始化 timer1
void timer1_init(void)
{
    // CTC 模式
    TCCR1B |= (1 << WGM12);

    // 每 1 ms
    OCR1A = 2499;

    // 开启 Compare 中断
    TIMSK1 |= (1 << OCIE1A);

    // 64 分频
    TCCR1B |= (1 << CS11) | (1 << CS10);
}

#define CONTEXT_SWITCH_ISR()                                                                                           \
    SAVE_CONTEXT()                                                                                                     \
    "call schedule_next\n\t" RESTORE_CONTEXT() "reti\n\t"

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
    //tick++;
    asm volatile (
        CONTEXT_SWITCH_ISR()
    );
}
