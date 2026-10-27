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

    task_init(&task1,"task1", task1_stack, TASK_STACK_SIZE, task1_func);
    task_init(&task2, "task2", task2_stack, TASK_STACK_SIZE, task2_func);
    task_init(&shell_task, "shell", shell_task_stack, TASK_STACK_SIZE, shell_task_func);

    // 空闲任务要放在最后初始化，使得其索引为 task_count - 1
    task_init(&idle_task, "idle", idle_task_stack, TASK_STACK_SIZE, idle_task_func);

    current_task = &task1;

    os_start_first();
}
