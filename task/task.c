#include <avr/interrupt.h>
#include <avr/io.h>

#include "../uart/uart.h"
#include "task.h"

#define MAX_TASK 3

static Task *task_table[MAX_TASK];
static uint8_t task_count = 0;
static uint8_t current_task_index = 0;

Task *current_task;
// 创建两个任务
Task task1;
Task task2;
Task shell_task;

uint8_t task1_stack[TASK_STACK_SIZE];
uint8_t task2_stack[TASK_STACK_SIZE];
uint8_t shell_task_stack[TASK_STACK_SIZE];

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
void task_init(Task *task, uint8_t *stack, uint16_t stack_size, void (*entry)(void))
{
    uint8_t *sp = &stack[stack_size - 1];

    // 函数入口地址
    uint16_t pc = (uint16_t)entry;

    stack_push(&sp, pc & 0xFF); // PC 低字节
    stack_push(&sp, pc >> 8);   // PC 高字节

    stack_push(&sp, 0x00); // r0 = 0
    stack_push(&sp, 0x00); // SREG (中断关闭)

    stack_push(&sp, 0x00); // r1 = 0

    for (uint8_t r = 2; r <= 31; r++)
    {
        stack_push(&sp, 0x00); // r2-r31 = 0
    }

    task->sp = (uint16_t)sp; // SP

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
        "ret\n\t"

    );
}

__attribute__((noinline)) void schedule_next(void)
{
    if (task_count == 0)
    {
        return;
    }

    current_task_index = (current_task_index + 1) % task_count;

    current_task = task_table[current_task_index];
}

// 切换任务
__attribute__((naked, noinline)) void os_yield(void)
{
    asm volatile(
        // 保存 r0
        "push r0\n\t"

        // 保存 SREG
        "in r0, __SREG__\n\t"
        "cli\n\t"
        "push r0\n\t"

        // 保存 r1
        "push r1\n\t"
        // 确保 r1 = 0
        "clr r1\n\t"

        // 保存 r2 - r31
        "push r2\n\t"
        "push r3\n\t"
        "push r4\n\t"
        "push r5\n\t"
        "push r6\n\t"
        "push r7\n\t"
        "push r8\n\t"
        "push r9\n\t"
        "push r10\n\t"
        "push r11\n\t"
        "push r12\n\t"
        "push r13\n\t"
        "push r14\n\t"
        "push r15\n\t"
        "push r16\n\t"
        "push r17\n\t"
        "push r18\n\t"
        "push r19\n\t"
        "push r20\n\t"
        "push r21\n\t"
        "push r22\n\t"
        "push r23\n\t"
        "push r24\n\t"
        "push r25\n\t"
        "push r26\n\t"
        "push r27\n\t"
        "push r28\n\t"
        "push r29\n\t"
        "push r30\n\t"
        "push r31\n\t"

        // 保存 SP 到 current_task->sp
        "in r24, __SP_L__\n\t"
        "in r25, __SP_H__\n\t"

        "lds r26, current_task\n\t"
        "lds r27, current_task+1\n\t"

        "st X+, r24\n\t"
        "st X, r25\n\t"

        "call schedule_next\n\t"

        "lds r26, current_task\n\t"
        "lds r27, current_task+1\n\t"

        "ld r24, X+\n\t"
        "ld r25, X\n\t"

        "out __SP_H__, r25\n\t"
        "out __SP_L__, r24\n\t"

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

        // 取出 PC
        "ret\n\t"

    );
}

void task1_func(void)
{
    sei();

    uart_puts("task1 running\r\n");

    while (1)
    {
        PORTB |= (1 << PB5);

        os_yield();
    }
}

void task2_func(void)
{
    sei();

    uart_puts("task1 running\r\n");

    while (1)
    {
        PORTB &= ~(1 << PB5);
        os_yield();
    }
}
