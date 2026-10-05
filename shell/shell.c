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

#include "shell.h"
#include "../timer/timer.h"
#include "../uart/uart.h"
#include "../task/task.h"

#define CMD_SIZE 32
#define MAX_ARG_SIZE 8

char cmd_buffer[CMD_SIZE];
uint8_t cmd_index = 0;

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

void shell_task_func(void)
{
    sei();
    shell_init();

    while(1)
    {
        char c;

        if (uart_try_getc(&c))
        {
            shell_input(c);
        }

        os_yield();
    }
}
