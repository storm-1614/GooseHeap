#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_STACK_SIZE 96

// 任务
typedef struct
{
    uint16_t sp; // 任务的 SP 值
} Task;

extern Task *current_task;
extern Task task1;
extern Task task2;
extern uint8_t task1_stack[TASK_STACK_SIZE];
extern uint8_t task2_stack[TASK_STACK_SIZE];

void task_init(Task *task, uint8_t *stack, uint16_t stack_size, void (*entry)(void));
__attribute__((naked, noreturn)) void os_start_first(void);
__attribute__((noinline)) void schedule_next(void);
__attribute__((naked, noinline)) void os_yield(void);
void task1_func(void);
void task2_func(void);

#endif
