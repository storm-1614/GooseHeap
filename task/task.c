#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "../timer/timer.h"
#include "../uart/uart.h"

#include "task.h"

static Task *task_table[MAX_TASK];
static uint8_t task_count = 0;
static uint8_t current_task_index = 0;

Task *current_task;
// 创建两个任务
Task task1;
Task task2;
Task shell_task;

Task idle_task;

uint8_t task1_stack[TASK_STACK_SIZE];
uint8_t task2_stack[TASK_STACK_SIZE];
uint8_t shell_task_stack[TASK_STACK_SIZE];
uint8_t idle_task_stack[TASK_STACK_SIZE];

// 入栈
static void stack_push(uint8_t **sp, uint8_t value)
{
    **sp = value; // 写栈
    (*sp)--;      // SP 向低地址移动
}

/* 初始化任务
 * 伪造初始上下文：
 * r0
 * SREG
 * r1
 * r2-r31
 */
void task_init(Task *task, const char *name, uint8_t *stack, uint16_t stack_size, void (*entry)(void))
{
    uint8_t *sp = &stack[stack_size - 1];

    // 函数入口地址
    uint16_t pc = (uint16_t)entry;

    stack_push(&sp, pc & 0xFF); // PC 低字节
    stack_push(&sp, pc >> 8);   // PC 高字节

    stack_push(&sp, 0x00); // r0 = 0
    stack_push(&sp, 0x00); // SREG.I = 0

    stack_push(&sp, 0x00); // r1 = 0

    for (uint8_t r = 2; r <= 31; r++)
    {
        stack_push(&sp, 0x00); // r2-r31 = 0
    }

    task->sp = (uint16_t)sp;  // SP
    task->name = name;        // 任务名
    task->state = TASK_READY; // 任务状态
    task->wake_tick = 0;      // 唤醒时间

    if (task_count < MAX_TASK)
    {
        task_table[task_count++] = task;
    }
}

__attribute__((naked, noreturn)) void os_start_first(void)
{
    asm volatile(
        // 关闭中断
        "cli\n\t"

        // task1.sp
        // task1        SP low
        // task1 + 1    SP high
        "lds r26, task1\n\t"
        "lds r27, task1+1\n\t"

        // CPU SP = task1.sp
        "out __SP_H__, r27\n\t"
        "out __SP_L__, r26\n\t"

        // 恢复寄存器
        "pop r31\n\t"
        "pop r30\n\t"
        "pop r29\n\t"
        "pop r28\n\t"
        "pop r27\n\t"
        "pop r26\n\t"
        "pop r25\n\t"
        "pop r24\n\t"
        "pop r23\n\t"
        "pop r22\n\t"
        "pop r21\n\t"
        "pop r20\n\t"
        "pop r19\n\t"
        "pop r18\n\t"
        "pop r17\n\t"
        "pop r16\n\t"
        "pop r15\n\t"
        "pop r14\n\t"
        "pop r13\n\t"
        "pop r12\n\t"
        "pop r11\n\t"
        "pop r10\n\t"
        "pop r9\n\t"
        "pop r8\n\t"
        "pop r7\n\t"
        "pop r6\n\t"
        "pop r5\n\t"
        "pop r4\n\t"
        "pop r3\n\t"
        "pop r2\n\t"
        "pop r1\n\t"

        // SREG
        "pop r0\n\t"
        "out __SREG__, r0\n\t"

        "pop r0\n\t"

        // 返回地址
        "reti\n\t"

    );
}

__attribute__((naked, noinline)) void os_switch_blocked(void)
{
    asm volatile(SAVE_CONTEXT("") "call schedule_next\n\t" RESTORE_CONTEXT() "reti\n\t");
}

// 任务睡眠
void os_sleep(uint32_t ms)
{
    // 防御式编程。防止 idle 意外调用 os_sleep
    if (ms == 0 || current_task == &idle_task)
        return;
    cli();

    current_task->wake_tick = tick + ms;
    current_task->state = TASK_BLOCKED;

    os_switch_blocked();

    asm volatile("" ::: "memory");
}

// 空闲任务
void idle_task_func(void)
{
    while (1)
    {
    }
}

__attribute__((noinline)) void schedule_next(void)
{
    for (uint8_t n = 0; n < task_count; n++)
    {
        uint8_t next = (current_task_index + 1 + n) % task_count;
        Task *candidate = task_table[next];

        if (candidate == &idle_task)
            continue;

        if (candidate->state == TASK_READY)
        {
            current_task_index = next;
            current_task = task_table[next];
            return;
        }
    }

    current_task_index = task_count - 1;
    current_task = &idle_task;
}

// 切换任务
__attribute__((naked, noinline)) void os_yield(void)
{
    asm volatile(SAVE_CONTEXT("") "call schedule_next\n\t" RESTORE_CONTEXT() "reti\n\t");
}

void task1_func(void)
{
    sei();

    uart_puts("task1 running\r\n");

    while (1)
    {
        PORTB |= (1 << PB5);

        os_sleep(500);
        //        os_yield();
    }
}
void task2_func(void)
{
    sei();

    uart_puts("task1 running\r\n");

    while (1)
    {
        PORTB &= ~(1 << PB5);
        os_sleep(1000);
        //        os_yield();
    }
}

void task_wake_expired(uint32_t now)
{
    for (int i = 0; i < task_count; i++)
    {
        Task *task = task_table[i];

        if (task->state == TASK_BLOCKED && (int32_t)(now - task->wake_tick) >= 0)
        {
            task->state = TASK_READY;
        }
    }
}

BOOL task_get_snapshot(uint8_t idx, Task *snapshot, BOOL *running, uint32_t *remaining)
{
    if (snapshot == NULL || running == NULL || remaining == NULL)
        return FALSE;

    if (idx >= task_count)
        return FALSE;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        *snapshot = *task_table[idx];
        *running = (task_table[idx] == current_task);

        if (*running)
        {
            snapshot->sp = SP;
        }

        *remaining = 0;

        if (snapshot->state == TASK_BLOCKED)
        {
            int32_t diff = (int32_t)(snapshot->wake_tick - tick);
            if (diff > 0)
            {
                *remaining = (uint32_t)diff;
            }
        }
    }
    return TRUE;
}
