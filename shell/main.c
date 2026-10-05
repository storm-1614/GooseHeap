/* GooseHeap Shell
 * 基于 UART 的 Arduino Shell
 *
 * Author: storm1614.top
 * Date: 2026-10-05
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <util/atomic.h>

#define FALSE 0
#define TRUE 1
#define BOOL int

#define CMD_SIZE 32
#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 64
#define MAX_ARG_SIZE 8

#define TASK_STACK_SIZE 96

volatile char rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

volatile char tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_head = 0;
volatile uint8_t tx_tail = 0;

volatile uint32_t tick = 0;

char cmd_buffer[CMD_SIZE];
uint8_t cmd_index = 0;

// 任务
typedef struct
{
    uint16_t sp; // 任务的 SP 值
} Task;

// 创建两个任务
Task task1;
Task task2;

uint8_t task1_stack[TASK_STACK_SIZE];
uint8_t task2_stack[TASK_STACK_SIZE];

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
}

void uart_init(void)
{
    // 9600 band
    UBRR0 = 103;

    // 开启发送、接收、接收完成中断
    UCSR0B |= (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

    // 8N1
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
}

void port_init(void)
{
    // 将 PB5 设置为输出
    DDRB |= (1 << PB5);
}

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

// 接收字符
BOOL uart_try_getc(char *c)
{
    // 如果 buffer 为空就返回
    if (rx_head == rx_tail)
        return FALSE;

    // 取出 tail 指向的字符
    *c = rx_buffer[rx_tail];

    // tail 前移
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;

    return TRUE;
}

void uart_putc(char c)
{
    uint8_t next = (tx_head + 1) % TX_BUFFER_SIZE;

    // 如果缓冲区满了就阻塞等待
    while (next == tx_tail)
    {
    }

    // 写入缓冲区
    tx_buffer[tx_head] = c;
    tx_head = next;

    // 置位 UDRIE0 启用 USART_UDRE 中断
    UCSR0B |= (1 << UDRIE0);
}

void uart_puts(const char *s)
{
    while (*s)
    {
        uart_putc(*s++);
    }
}

// 发送 u32
void uart_put_u32(uint32_t n)
{
    char buf[10];
    uint8_t i = 0;
    if (n == 0)
    {
        buf[0] = '0';
        i = 1;
    }

    while (n > 0)
    {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }

    while (i > 0)
    {
        uart_putc(buf[--i]);
    }
}

void uart_put_hex_digit(uint8_t n)
{
    if (n < 10)
    {
        uart_putc('0' + n);
    }
    else
    {
        uart_putc('A' + (n - 10));
    }
}

void uart_put_hex16(uint16_t n)
{
    uart_puts("0x");

    uart_put_hex_digit((n >> 12) & 0xF);
    uart_put_hex_digit((n >> 8) & 0xF);
    uart_put_hex_digit((n >> 4) & 0xF);
    uart_put_hex_digit(n & 0xF);
}

void task1_func(void)
{
    sei();

    uart_puts("task1 running\r\n");

    PORTB |= (1 << PB5);

    while (1)
    {
        // TODO
    }
}

void task2_func(void)
{
    while (1)
    {
        // TODO
    }
}

void shell_init()
{
    uart_puts("Shell initing...\r\n");
    uart_puts("GooseHeap Shell\r\n");
    uart_puts("> ");
}

void cmd_help(void)
{
    uart_puts("commands:\r\n");
    uart_puts("help\r\n");
    uart_puts("led on\r\n");
    uart_puts("led off\r\n");
    uart_puts("tick\r\n");
}

void cmd_led(uint8_t argc, char **argv)
{
    if (argc != 1)
    {
        uart_puts("usage: led <on|off>\r\n");
        return;
    }

    if (strcmp(argv[0], "on") == 0)
    {
        PORTB |= (1 << PB5);
        uart_puts("LED ON\r\n");
    }
    else if (strcmp(argv[0], "off") == 0)
    {
        PORTB &= ~(1 << PB5);
        uart_puts("LED OFF\r\n");
    }
    else
    {
        uart_puts("unknown command\r\n");
        uart_puts("usage: led <on|off>\r\n");
    }
}

void cmd_tick(void)
{
    uint32_t snapshot;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        snapshot = tick;
    }
    uart_puts("uptime = ");
    uart_put_u32(snapshot);
    uart_puts("ms\r\n");
}

void cmd_uptime(void)
{
    uint32_t snapshot;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        snapshot = tick;
    }
    uart_puts("uptime = ");
    uart_put_u32(snapshot);
    uart_puts(" ms\r\n");
}

void cmd_sp(void)
{
    uint16_t sp = SP;

    uart_puts("SP = ");
    uart_put_hex16(sp);
    uart_puts("\r\n");
}

__attribute__((noinline)) void bar(void)
{
    uint16_t sp = SP;

    uart_puts("bar:   ");
    uart_put_hex16(sp);
    uart_puts("\r\n");
}

__attribute__((noinline)) void foo(void)
{
    uint16_t sp = SP;

    uart_puts("foo:   ");
    uart_put_hex16(sp);
    uart_puts("\r\n");

    bar();

    uart_puts("bar returned\r\n");
}

void cmd_stack(void)
{
    uint16_t sp = SP;

    uart_puts("cmd:  ");
    uart_put_hex16(sp);
    uart_puts("\r\n");

    foo();

    uart_puts("foo returned\r\n");
}

int parse_args(char *line, char **argv, int max_args)
{
    int count = 0;
    char *p = line;

    while (*p && count < max_args - 1)
    {
        // 跳过空格
        while (*p == ' ')
        {
            p++;
        }

        if (*p == '\0')
            break;

        // 参数
        argv[count++] = p;

        // 遍历参数到末尾
        while (*p != ' ' && *p != '\0')
        {
            p++;
        }

        // 切断
        if (*p == ' ')
        {
            *p = '\0';
            p++;
        }
    }

    argv[count] = NULL;

    return count;
}

// 解析命令并执行
void shell_execute(char *cmd)
{
    char *arg[MAX_ARG_SIZE];
    uint8_t argc = parse_args(cmd, arg, MAX_ARG_SIZE);

    if (argc == 0)
    {
        return;
    }

    if (strcmp(arg[0], "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(arg[0], "led") == 0)
    {
        cmd_led(argc - 1, arg + 1);
    }
    else if (strcmp(arg[0], "tick") == 0)
    {
        cmd_tick();
    }
    else if (strcmp(arg[0], "uptime") == 0)
    {
        cmd_uptime();
    }
    else if (strcmp(arg[0], "sp") == 0)
    {
        cmd_sp();
    }
    else if (strcmp(arg[0], "stack") == 0)
    {
        cmd_stack();
    }
    else
    {
        uart_puts("unknown command\r\n");
    }
}

// 处理输入
void shell_input(char c)
{
    if (c == '\b' || c == 0x7F)
    {
        if (cmd_index > 0)
        {
            cmd_index--;
            uart_puts("\b \b");
        }
    }
    else if (c == '\r')
    {
        cmd_buffer[cmd_index] = '\0';

        uart_puts("\r\n");
        shell_execute(cmd_buffer);

        cmd_index = 0;

        uart_puts("> ");
    }
    else if (c == '\n')
    {
    }
    else
    {
        if (cmd_index < CMD_SIZE - 1)
        {
            cmd_buffer[cmd_index] = c;
            cmd_index++;
            uart_putc(c);
        }
    }
}

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

// UART 接收 中断
ISR(USART_RX_vect)
{
    char c = UDR0;

    uint8_t next = (rx_head + 1) % RX_BUFFER_SIZE;

    if (next != rx_tail)
    {
        rx_buffer[rx_head] = c;
        rx_head = next;
    }
}

// UART UDR0 空中断
ISR(USART_UDRE_vect)
{
    if (tx_head == tx_tail)
    {
        UCSR0B &= ~(1 << UDRIE0);
        return;
    }

    UDR0 = tx_buffer[tx_tail];
    tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
}

ISR(TIMER1_COMPA_vect)
{
    tick++;
}
