#include "delay.h"
#include "FreeRTOS.h"

#define MSEC_TO_TICK(msec)  (((uint32_t)(msec) + 500uL / (uint32_t)configTICK_RATE_HZ) \
                            * (uint32_t)configTICK_RATE_HZ / 1000uL)
#define TICKS_TO_MSEC(tick) ((tick)*1000uL/(uint32_t)configTICK_RATE_HZ)

// msº∂—” ±
void delay_ms(int ms)
{
    vTaskDelay(MSEC_TO_TICK(ms));
}
