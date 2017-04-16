#include "i2c.h"
#include <stdio.h>

#define TIMEOUT 100000u

uint8_t I2C_ReadByte(I2C_Type * base, uint8_t *res)
{
    uint32_t timeout = TIMEOUT;
    
    /* Read byte from I2C data register. */
    uint8_t byte = I2C_RD_D(base);

    /* Wait till byte transfer complete. */
    while (!I2C_BRD_S_IICIF(base) && timeout--) {
        __asm volatile("nop");
    }
    
    if (!I2C_BRD_S_IICIF(base)) { // 超时
        *res = 1;
        printf("IIC read timeout.\n");
    } else {
        *res = 0;
    }

    /* Clear interrupt flag */
    I2C_BWR_S_IICIF(base, 0x1U);

    return byte;
}

// IIC读取字节
bool I2C_WriteByte(I2C_Type * base, uint8_t byte, uint8_t *res)
{
    uint32_t timeout = TIMEOUT;
    
    /* Write byte into I2C data register. */
    I2C_WR_D(base, byte);

    /* Wait till byte transfer complete. */
    while (!I2C_BRD_S_IICIF(base) && timeout) {
        __asm volatile("nop");
    }
    
    if (!I2C_BRD_S_IICIF(base)) { // 超时
        *res = 1;
        printf("IIC write timeout.\n");
    } else {
        *res = 0;
    }

    /* Clear interrupt flag */
    I2C_BWR_S_IICIF(base, 0x1U);

    /* Return 0 if no acknowledge is detected. */
    return !I2C_BRD_S_RXAK(base);
}

i2c_status_t I2C_ReceiveBuffer(I2C_Type * base,
                               uint16_t slaveAddr,
                               const uint8_t cmd,
                               uint8_t * rxBuff,
                               uint32_t rxSize)
{
    int32_t i;
    uint8_t address, res;

    /* Return if current I2C is already set as master. */
    if (I2C_BRD_C1_MST(base)) {
        return kStatus_I2C_Busy;
    }

    address = (uint8_t)slaveAddr << 1U;

    /* Set direction to send */
    I2C_HAL_SetDirMode(base, kI2CSend);

    /* Generate START signal. */
    I2C_HAL_SendStart(base);

    /* Send slave address */
    if (!I2C_WriteByte(base, address, &res)) {
        /* Send STOP if no ACK received */
        I2C_HAL_SendStop(base);
        return kStatus_I2C_ReceivedNak;
    }
    if (res) { // 超时就返回
        return kStatus_I2C_Timeout;
    }

    // 发送命令
    if (!I2C_WriteByte(base, cmd, &res)) {
        /* Send STOP if no ACK received */
        I2C_HAL_SendStop(base);
        return kStatus_I2C_ReceivedNak;
    }
    if (res) { // 超时就返回
        return kStatus_I2C_Timeout;
    }
    
    /* Need to generate a repeat start before changing to receive. */
    I2C_HAL_SendStart(base);
    
    /* Send slave address again */
    if (!I2C_WriteByte(base, address | 1U, &res)) {
       /* Send STOP if no ACK received */
       I2C_HAL_SendStop(base);
       return kStatus_I2C_ReceivedNak;
    }
    if (res) { // 超时就返回
        return kStatus_I2C_Timeout;
    }

    /* Change direction to receive. */
    I2C_HAL_SetDirMode(base, kI2CReceive);

    /* Send NAK if only one byte to read. */
    if (rxSize == 0x1U) {
        I2C_HAL_SendNak(base);
    } else {
        I2C_HAL_SendAck(base);
    }

    /* Dummy read to trigger receive of next byte. */
    I2C_ReadByte(base, &res);
    if (res) { // 超时就返回
        return kStatus_I2C_Timeout;
    }

    /* Receive data */
    for(i = rxSize - 1; i >= 0; i--) {
        switch (i) {
            case 0x0U:
                /* Generate STOP signal. */
                I2C_HAL_SendStop(base);
                break;
            case 0x1U:
                /* For the byte before last, we need to set NAK */
                I2C_HAL_SendNak(base);
                break;
            default :
                I2C_HAL_SendAck(base);
                break;
        }

        /* Read recently received byte into buffer and update buffer index */
        if (i==0) {
            *rxBuff++ = I2C_HAL_ReadByte(base);
        } else {
            *rxBuff++ = I2C_ReadByte(base, &res);
            if (res) { // 超时就返回
                return kStatus_I2C_Timeout;
            }
        }
    }
    return kStatus_I2C_Success;
}
