/************************************************************************************
* File name: oled_i2c.c
* Processor: ATmeag8
* Compiler : ICCAVR
*
*************************************************************************************/

#define CONST const

#include "oled_i2c.h"
#include "oled_codetab.h"

#include "includes.h"


//通过调整0R电阻,屏可以0x78和0x7A两个地址 -- 默认0x78
#define OLED_ADDRESS	0x78

static void I2C_Configuration(void)
{
    // OLED的I2C接口不使用浮空输入, 这样可以避免在OLED拔出的时候死机
    SIM_HAL_EnableClock(SIM, kSimClockGatePortD);
    PORT_HAL_SetMuxMode(PORTD, 8, kPortMuxAlt2);
    PORT_HAL_SetMuxMode(PORTD, 9, kPortMuxAlt2);
    
    SIM_HAL_EnableClock(SIM, kSimClockGateI2c0);
    I2C_HAL_Init(I2C0);
    I2C_HAL_Enable(I2C0);
    I2C_HAL_SetBaudRate(I2C0, BSP_GetBusClockFreq(), 400, NULL);
    I2C_HAL_SetDirMode(I2C0, kI2CSend);
    I2C_HAL_ClearInt(I2C0);
}

static void I2C_WriteByte(unsigned char Byte)
{
    I2C_HAL_WriteByteBlocking(I2C0, Byte);
}

static void I2C_Stop(void)
{
    I2C_HAL_SendStop(I2C0);
}

/*********************OLED写数据************************************/ 
static void WriteDat(void)
{
    I2C_HAL_SendStart(I2C0);
    I2C_HAL_WriteByteBlocking(I2C0, OLED_ADDRESS);
    I2C_HAL_WriteByteBlocking(I2C0, 0x40);
}

/*********************OLED写命令************************************/
static void WriteCmd(void)
{
    I2C_HAL_SendStart(I2C0);
    I2C_HAL_WriteByteBlocking(I2C0, OLED_ADDRESS);
    I2C_HAL_WriteByteBlocking(I2C0, 0x00);
}

/*********************OLED初始化************************************/
void OLED_Init(void)
{
    I2C_Configuration();
    WriteCmd();
    I2C_WriteByte(0xae); // --turn off oled panel
    I2C_WriteByte(0x00); // ---set low column address
    I2C_WriteByte(0x10); // ---set high column address
    I2C_WriteByte(0x40); // --set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    I2C_WriteByte(0x81); // --set contrast control register
    I2C_WriteByte(0xcf); // Set SEG Output Current Brightness
    I2C_WriteByte(0xa1); // --Set SEG/Column Mapping     0xa0左右反置 0xa1正常
    I2C_WriteByte(0xc8); // Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
    I2C_WriteByte(0xa6); // --set normal display
    I2C_WriteByte(0xa8); // --set multiplex ratio(1 to 64)
    I2C_WriteByte(0x3f); // --1/64 duty
    I2C_WriteByte(0xd3); // -set display offset	Shift Mapping RAM Counter (0x00~0x3F)
    I2C_WriteByte(0x00); // -not offset
    I2C_WriteByte(0xd5); // --set display clock divide ratio/oscillator frequency
    I2C_WriteByte(0x80); // --set divide ratio, Set Clock as 100 Frames/Sec
    I2C_WriteByte(0xd9); // --set pre-charge period
    I2C_WriteByte(0xf1); // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    I2C_WriteByte(0xda); // --set com pins hardware configuration
    I2C_WriteByte(0x12);
    I2C_WriteByte(0xdb); // --set vcomh
    I2C_WriteByte(0x40); // Set VCOM Deselect Level
    I2C_WriteByte(0x20); // -Set Page Addressing Mode (0x00/0x01/0x02)
    I2C_WriteByte(0x02); //
    I2C_WriteByte(0x8d); // --set Charge Pump enable/disable
    I2C_WriteByte(0x14); // --set(0x10) disable
    I2C_WriteByte(0xa4); // Disable Entire Display On (0xa4/0xa5)
    I2C_WriteByte(0xa6); // Disable Inverse Display On (0xa6/a7) 
    I2C_WriteByte(0xaf); // --turn on oled panel
    OLED_Fill(0x00);
}
void OLED_SetPos(unsigned char x, unsigned char y) //设置起始点坐标
{
    WriteCmd();
    I2C_WriteByte(0xb0 | y);
    I2C_WriteByte(((x & 0xf0) >> 4) | 0x10);
    I2C_WriteByte(x & 0x0f);
    I2C_Stop();
}

//全屏填充
void OLED_Fill(unsigned char data)
{
    unsigned char y, x;

    for (y = 0; y < 8; ++y) {
        WriteCmd();
        I2C_WriteByte(0xb0 | y);
        I2C_WriteByte(0x10);
        I2C_WriteByte(0x00);
        WriteDat();
        for (x = 0; x < 128; ++x) {
            I2C_WriteByte(data);
        }
    }
    I2C_Stop();
}

void OLED_Cls(void)//清屏
{
    OLED_Fill(0x00);
}

//--------------------------------------------------------------
// Prototype      : void OLED_ON(void)
// Calls          : 
// Parameters     : none
// Description    : 将OLED从休眠中唤醒
//--------------------------------------------------------------
void OLED_PowerOn(void)
{
    WriteCmd();
    I2C_WriteByte(0X8D);  //设置电荷泵
    I2C_WriteByte(0X14);  //开启电荷泵
    I2C_WriteByte(0XAF);  //OLED唤醒
    I2C_Stop();
}

//--------------------------------------------------------------
// Prototype      : void OLED_OFF(void)
// Calls          : 
// Parameters     : none
// Description    : 让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
//--------------------------------------------------------------
void OLED_PowerOff(void)
{
    WriteCmd();
    I2C_WriteByte(0X8D);  //设置电荷泵
    I2C_WriteByte(0X10);  //关闭电荷泵
    I2C_WriteByte(0XAE);  //OLED休眠
    I2C_Stop();
}

/* 显示6x8字符 */
void OLED_P6x8Char(unsigned char x, unsigned char y, char ch)
{
	unsigned char i;
    const unsigned char *pTab = F6x8[ch - 32];

    OLED_SetPos(x, y);
    WriteDat();
    for (i = 0; i < 6; i++) {
        I2C_WriteByte(*pTab++);
    }
    OLED_SetPos(x, y + 1);
    WriteDat();
    for (i = 0; i < 6; ++i) {
        I2C_WriteByte(*pTab++);
    }
    I2C_Stop();
}

/* 显示8x16字符 */
void OLED_P8x16Char(unsigned char x, unsigned char y, char ch)
{
	unsigned char i;
    const unsigned char *pTab = F8X16 + (int)(ch - 32) * 16;
    
    OLED_SetPos(x, y);
    WriteDat();
    for (i = 0; i < 8; i++) {
        I2C_WriteByte(*pTab++);
    }
    OLED_SetPos(x, y + 1);
    WriteDat();
    for (i = 0; i < 8; ++i) {
        I2C_WriteByte(*pTab++);
    }
    I2C_Stop();
}

void OLED_DispStr6x8(unsigned char x, unsigned char y, const char *str)
{
    while (*str != '\0') {
        if (x > 120) {
            x = 0;
            y += 1;
        }
        OLED_P6x8Char(x, y, *str++);
        x += 6;
    }
}

void OLED_DispStr8x16(unsigned char x, unsigned char y, const char *str)
{
    while (*str != '\0') {
        if (x > 120) {
            x = 0;
            y += 2;
        }
        OLED_P8x16Char(x, y, *str++);
        x += 8;
    }
}

void OLED_DrawBitmap(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1,unsigned char *bmp)
{
	unsigned int j = 0;
	unsigned char x,y;

    if (y1 % 8 == 0) {
        y = y1 / 8;
    } else {
        y = y1 / 8 + 1;
    }
	for(y = y0; y < y1; y++) {
		OLED_SetPos(x0, y);
        WriteDat();
        for(x = x0; x < x1; x++) {
	    	I2C_WriteByte(bmp[j++]);
	    }
        I2C_Stop();
	}
}
