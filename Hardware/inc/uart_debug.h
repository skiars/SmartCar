#ifndef __UART_DEBUG_H
#define __UART_DEBUG_H

#include <stdint.h>

void uart_debug_init(int baudRate);
void uart_senddata(const char *txBuff, uint32_t txSize);

#endif
