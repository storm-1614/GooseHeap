/* uart.h
 *
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "../include/bool.h"

void uart_init(void);
BOOL uart_try_getc(char *c);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_put_u32(uint32_t n);
void uart_put_hex_digit(uint8_t n);
void uart_put_hex16(uint16_t n);

#endif
