#include "includes.h"
#include "OV7725.h"
#include "fsl_uart_hal.h"

// 内联的字符串发送函数, 用于系统错误时紧急发送调试信息
static inline void uart_send_str_inline(char *str)
{
    while (*str) {
        UART1->D = (uint8_t)*str++;
        while (!UART_BRD_S1_TDRE(UART1)); // 等待传输完成
    }
}

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
    uart_send_str_inline("\nNMI_Handler!\n");
    while (1);
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
    uart_send_str_inline("\nHardFault_Handler!\n");
    while (1);
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
    uart_send_str_inline("\nMemManage_Handler!\n");
    while (1);
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
    uart_send_str_inline("\nBusFault_Handler!\n");
    while (1);
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
    uart_send_str_inline("\nUsageFault_Handler!\n");
    while (1);
}

// DMA0传输完成中断
void DMA0_DMA16_IRQHandler(void)
{
    ov7725_eagle_dma();
}

// PTA外部中断
void PORTA_IRQHandler(void)
{
    uint32_t isfr = PORTA->ISFR;
    
    PORTA->ISFR  = ~0; // 清除中断标志
    // 确定为PTA29引脚触发的中断时才进入场中断
    if (isfr & (1 << 29)) {
        ov7725_eagle_vsync();
    }
}
