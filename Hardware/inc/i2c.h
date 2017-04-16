#ifndef __I2C_H
#define __I2C_H

#include "fsl_i2c_hal.h"

i2c_status_t I2C_ReceiveBuffer(I2C_Type * base,
    uint16_t slaveAddr, const uint8_t cmd, uint8_t * rxBuff, uint32_t rxSize);

#endif
