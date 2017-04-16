#ifndef __OLED_I2C_H
#define	__OLED_I2C_H

void OLED_Init(void);
void OLED_Cls(void);
void OLED_Fill(unsigned char bmp_dat);
void OLED_SetPos(unsigned char x, unsigned char y);
void OLED_PowerOn(void);
void OLED_PowerOff(void);
void OLED_P6x8Char(unsigned char x, unsigned char y, char ch);
void OLED_P8x16Char(unsigned char x, unsigned char y, char ch);
void OLED_DispStr6x8(unsigned char x, unsigned char y, const char *str);
void OLED_DispStr8x16(unsigned char x, unsigned char y, const char *str);
void OLED_DrawBitmap(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1,unsigned char *bmp);

#endif
