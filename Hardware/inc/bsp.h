#ifndef __BSP_H
#define __BSP_H

#include <stdint.h>

#include "OV7725.h"

extern uint8_t image_buff[CAMERA_SIZE];

// µç»úPWM·¶Î§
#define MOTER_PWM_RANGE     5000u

#ifndef LEFT
  #define LEFT                0u
#endif

#ifndef RIGHT
  #define RIGHT               1u
#endif

#define KEY1 0
#define KEY2 1
#define KEY3 2
#define KEY4 3

void board_init(void);
void BSP_MoterEnable(void);
void BSP_MoterDisable(void);
uint32_t BSP_GetBusClockFreq(void);
int BSP_GetEncoderCount(int left);
void BSP_SetMotorSpeed(int left, int value);
void BSP_MoterThresholdTest(void);
void BSP_MoterSpeedTest(void);
void BSP_RemoteProcess(void);
void BSP_BuzzerSwitch(int8_t sw);
float BSP_GetBatteryVoltage(void);
uint8_t BSP_GetKeyValue(uint8_t key);

#endif
