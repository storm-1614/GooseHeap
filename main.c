/* GooseHeap OS
 */

#include <avr/interrupt.h>

#include "kernel/port.h"
#include "shell/shell.h"
#include "task/task.h"
#include "timer/timer.h"
#include "uart/uart.h"

int main(void)
{
    // 初始化工作
    uart_init();
    port_init();
    timer1_init();

    task_init(&task1, task1_stack, TASK_STACK_SIZE, task1_func);
    task_init(&task2, task2_stack, TASK_STACK_SIZE, task2_func);

    os_start_first();

    sei(); // 启用全局中断

    // DEBUG
    uart_puts("task1.sp = ");
    uart_put_hex16(task1.sp);
    uart_puts("\r\n");
    uart_puts("task2.sp = ");
    uart_put_hex16(task2.sp);
    uart_puts("\r\n");

    shell_init();

    while (1)
    {
        char c;
        if (uart_try_getc(&c))
        {
            shell_input(c);
        }
    }
}
