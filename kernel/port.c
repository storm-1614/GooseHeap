#include <avr/io.h>

#include "port.h"

void port_init(void)
{
    // 将 PB5 设置为输出
    DDRB |= (1 << PB5);
}
