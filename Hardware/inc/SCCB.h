#ifndef __SCCB_H
#define __SCCB_H

#include <stdint.h>

#define SCL_H()         GPIO_HAL_SetPinOutput(PTA, 26)
#define SCL_L()         GPIO_HAL_ClearPinOutput(PTA, 26)
#define	SCL_DDR_OUT() 	GPIO_HAL_SetPinDir(PTA, 26, kGpioDigitalOutput)
#define	SCL_DDR_IN() 	GPIO_HAL_SetPinDir(PTA, 26, kGpioDigitalInput)

#define SDA_H()         GPIO_HAL_SetPinOutput(PTA, 25)
#define SDA_L()         GPIO_HAL_ClearPinOutput(PTA, 25)
#define SDA_IN()      	GPIO_HAL_ReadPinInput(PTA, 25)
#define SDA_DDR_OUT()	GPIO_HAL_SetPinDir(PTA, 25, kGpioDigitalOutput)
#define SDA_DDR_IN()	GPIO_HAL_SetPinDir(PTA, 25, kGpioDigitalInput)

#define ADDR_OV7725   0x42

// SCCB延时不足会导致初始化失败
#define SCCB_DELAY()    SCCB_delay(5000)


void SCCB_GPIO_init(void);
int SCCB_WriteByte( uint16_t WriteAddress , uint8_t SendByte);
int SCCB_ReadByte(uint8_t* pBuffer, uint16_t length, uint8_t ReadAddress);

static void SCCB_delay(uint16_t i);
#endif 
