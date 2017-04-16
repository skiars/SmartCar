#include "uart_debug.h"
#include "fsl_uart_hal.h"
#include "fsl_sim_hal.h"
#include "fsl_port_hal.h"
#include <stdio.h>

void uart_debug_init(int baudRate)
{
    SIM_HAL_EnableClock(SIM, kSimClockGateUart1);
    SIM_HAL_EnableClock(SIM, kSimClockGatePortC);
    PORT_HAL_SetMuxMode(PORTC, 3, kPortMuxAlt3);
    PORT_HAL_SetMuxMode(PORTC, 4, kPortMuxAlt3);
    
    UART_HAL_Init(UART1);
    UART_HAL_SetBaudRate(UART1, SystemCoreClock, baudRate); // UART1直接使用SystemCoreClock
    UART_HAL_SetBitCountPerChar(UART1, kUart8BitsPerChar);
    UART_HAL_SetParityMode(UART1, kUartParityDisabled);
    UART_HAL_SetStopBitCount(UART1, kUartOneStopBit);
    UART_HAL_EnableTransmitter(UART1);
    UART_HAL_EnableReceiver(UART1);
    
    // 接收中断使能
    UART_HAL_SetIntMode(UART1, kUartIntRxDataRegFull, true);
    NVIC_EnableIRQ(UART1_RX_TX_IRQn);
}

int fputc(int ch, FILE *f)
{
  UART_HAL_Putchar(UART1, (uint8_t)ch);
  while (!UART_BRD_S1_TDRE(UART1)); // 等待传输完成
  return (ch);
}

void uart_senddata(const char *txBuff, uint32_t txSize)
{
    UART_HAL_SendDataPolling(UART1, (const uint8_t *)txBuff, txSize);
}
