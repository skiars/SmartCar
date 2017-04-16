#include "includes.h"
#include "SCCB.h"

#define DEV_ADR  ADDR_OV7725 			 /*设备地址定义*/

/*******************************************************************************
* Function Name  : SCCB_delay
* Description    : 延迟时间
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void SCCB_delay(uint16_t i)
{	
    while(i) { 
        i--;
        __asm volatile("nop");
    } 
}


/********************************************************************
 * 函数名：SCCB_Configuration
 * 描述  ：SCCB管脚配置
 * 输入  ：无
 * 输出  ：无
 * 注意  ：无        
 ********************************************************************/
void SCCB_GPIO_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    
    //初始化SDA
    PORT_HAL_SetPullMode(PORTA, 25, kPortPullDown); // 上拉
    PORT_HAL_SetMuxMode(PORTA, 25, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTA, 25, kGpioDigitalOutput);
    
    //初始化SCL
    PORT_HAL_SetPullMode(PORTA, 26, kPortPullDown); // 上拉
    PORT_HAL_SetMuxMode(PORTA, 26, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTA, 26, kGpioDigitalOutput);
}


/********************************************************************
 * 函数名：SCCB_Start
 * 描述  ：SCCB起始信号
 * 输入  ：无
 * 输出  ：无
 * 注意  ：内部调用        
 ********************************************************************/
static uint8_t SCCB_Start(void)
{
    SDA_H();
    SCL_H();
    SCCB_DELAY();

    SDA_DDR_IN();
    SCCB_DELAY();
    if(!SDA_IN())
    {
        SDA_DDR_OUT();
        return 0;   /* SDA线为低电平则总线忙,退出 */
    }
    SDA_DDR_OUT();
    SDA_L();

    SCCB_DELAY();
    SCL_L();

    if(SDA_IN())
    {
        SDA_DDR_OUT();
        return 0;   /* SDA线为高电平则总线出错,退出 */
    }
    //SDA_DDR_OUT();
    //SDA_L();
    //SCCB_delay();
    return 1;
}



/********************************************************************
 * 函数名：SCCB_Stop
 * 描述  ：SCCB停止信号
 * 输入  ：无
 * 输出  ：无
 * 注意  ：内部调用        
 ********************************************************************/
static void SCCB_Stop(void)
{
	SCL_L();
    //SCCB_DELAY();
    SDA_L();
    SCCB_DELAY();
    SCL_H();
    SCCB_DELAY();
    SDA_H();
    SCCB_DELAY();
}



/********************************************************************
 * 函数名：SCCB_Ack
 * 描述  ：SCCB应答方式
 * 输入  ：无
 * 输出  ：无
 * 注意  ：内部调用        
 ********************************************************************/
static void SCCB_Ack(void)
{	
	SCL_L();
    SCCB_DELAY();
    SDA_L();
    SCCB_DELAY();
    SCL_H();
    SCCB_DELAY();
    SCL_L();
    SCCB_DELAY();
}



/********************************************************************
 * 函数名：SCCB_NoAck
 * 描述  ：SCCB 无应答方式
 * 输入  ：无
 * 输出  ：无
 * 注意  ：内部调用        
 ********************************************************************/
static void SCCB_NoAck(void)
{	
	SCL_L();
    SCCB_DELAY();
    SDA_H();
    SCCB_DELAY();
    SCL_H();
    SCCB_DELAY();
    SCL_L();
    SCCB_DELAY();
}

/********************************************************************
 * 函数名：SCCB_WaitAck
 * 描述  ：SCCB 等待应答
 * 输入  ：无
 * 输出  ：返回为:=1有ACK,=0无ACK
 * 注意  ：内部调用        
 ********************************************************************/
static int SCCB_WaitAck(void) 	
{
	SCL_L();
    //SDA_H();
    SDA_DDR_IN();

    SCCB_DELAY();
    SCL_H();

    SCCB_DELAY();

    if(SDA_IN())           //应答为高电平，异常，通信失败
    {
        SDA_DDR_OUT();
        SCL_L();
        return 0;
    }
    SDA_DDR_OUT();
    SCL_L();
    return 1;
}



 /*******************************************************************
 * 函数名：SCCB_SendByte
 * 描述  ：数据从高位到低位
 * 输入  ：SendByte: 发送的数据
 * 输出  ：无
 * 注意  ：内部调用        
 *********************************************************************/
static void SCCB_SendByte(uint8_t SendByte)
{
    uint8_t i = 8;
    while(i--)
    {

        if(SendByte & 0x80)     //SDA 输出数据
        {
            SDA_H();
        }
        else
        {
            SDA_L();
        }
        SendByte <<= 1;
        SCCB_DELAY();
        SCL_H();                //SCL 拉高，采集信号
        SCCB_DELAY();
        SCL_L();                //SCL 时钟线拉低
        //SCCB_DELAY();
    }
    //SCL_L();
}

 /******************************************************************
 * 函数名：SCCB_ReceiveByte
 * 描述  ：数据从高位到低位
 * 输入  ：无
 * 输出  ：SCCB总线返回的数据
 * 注意  ：内部调用        
 *******************************************************************/
static int SCCB_ReceiveByte(void)
{
    uint8_t i = 8;
    uint8_t ReceiveByte = 0;

    //SDA_H();
    //SCCB_DELAY();
    SDA_DDR_IN();

    while(i--)
    {
        ReceiveByte <<= 1;
        SCL_L();
        SCCB_DELAY();
        SCL_H();
        SCCB_DELAY();

        if(SDA_IN())
        {
            ReceiveByte |= 0x01;
        }


    }
    SDA_DDR_OUT();
    SCL_L();
    return ReceiveByte;
}

 /*****************************************************************************************
 * 函数名：SCCB_WriteByte
 * 描述  ：写一字节数据
 * 输入  ：- WriteAddress: 待写入地址 	- SendByte: 待写入数据	- DeviceAddress: 器件类型
 * 输出  ：返回为:=1成功写入,=0失败
 * 注意  ：无        
 *****************************************************************************************/           
static int SCCB_WriteByte_one( uint16_t WriteAddress , uint8_t SendByte );


int SCCB_WriteByte( uint16_t WriteAddress , uint8_t SendByte )            //考虑到用sccb的管脚模拟，比较容易失败，因此多试几次
{
    uint8_t i = 0;
    while( 0 == SCCB_WriteByte_one ( WriteAddress, SendByte ) )
    {
        i++;
        if(i == 20)
        {
            return 0 ;
        }
    }
    return 1;
}

int SCCB_WriteByte_one( uint16_t WriteAddress , uint8_t SendByte )
{
    if(!SCCB_Start())
    {
        return 0;
    }
    SCCB_SendByte( DEV_ADR );                    /* 器件地址 */
    if( !SCCB_WaitAck() )
    {
        SCCB_Stop();
        return 0;
    }
    SCCB_SendByte((uint8_t)(WriteAddress & 0x00FF));   /* 设置低起始地址 */
    SCCB_WaitAck();
    SCCB_SendByte(SendByte);
    SCCB_WaitAck();
    SCCB_Stop();
    return 1;
}

/******************************************************************************************************************
 * 函数名：SCCB_ReadByte
 * 描述  ：读取一串数据
 * 输入  ：- pBuffer: 存放读出数据 	- length: 待读出长度	- ReadAddress: 待读出地址		 - DeviceAddress: 器件类型
 * 输出  ：返回为:=1成功读入,=0失败
 * 注意  ：无        
 **********************************************************************************************************************/           
static int SCCB_ReadByte_one(uint8_t *pBuffer,   uint16_t length,   uint8_t ReadAddress);

int SCCB_ReadByte(uint8_t *pBuffer,   uint16_t length,   uint8_t ReadAddress)
{
    uint8_t i = 0;
    while( 0 == SCCB_ReadByte_one(pBuffer, length, ReadAddress) )
    {
        i++;
        if(i == 30)
        {
            return 0 ;
        }
    }
    return 1;
}

int SCCB_ReadByte_one(uint8_t *pBuffer,   uint16_t length,   uint8_t ReadAddress)
{
    if(!SCCB_Start())
    {
        return 0;
    }
    SCCB_SendByte( DEV_ADR );         /* 器件地址 */
    if( !SCCB_WaitAck() )
    {
        SCCB_Stop();
        return 0;
    }
    SCCB_SendByte( ReadAddress );           /* 设置低起始地址 */
    SCCB_WaitAck();
    SCCB_Stop();

    if(!SCCB_Start())
    {
        return 0;
    }
    SCCB_SendByte( DEV_ADR + 1 );               /* 器件地址 */

    if(!SCCB_WaitAck())
    {
        SCCB_Stop();
        return 0;
    }
    while(length)
    {
        *pBuffer = SCCB_ReceiveByte();
        if(length == 1)
        {
            SCCB_NoAck();
        }
        else
        {
            SCCB_Ack();
        }
        pBuffer++;
        length--;
    }
    SCCB_Stop();
    return 1;
}




