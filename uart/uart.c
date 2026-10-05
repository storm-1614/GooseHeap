/* uart.c
 * 
 */

#include <avr/interrupt.h>
#include <avr/io.h>

#include "uart.h"

#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 64

volatile char rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

volatile char tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_head = 0;
volatile uint8_t tx_tail = 0;

void uart_init(void)
{
    // 9600 band
    UBRR0 = 103;

    // 开启发送、接收、接收完成中断
    UCSR0B |= (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

    // 8N1
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
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
