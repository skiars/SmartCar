#ifndef __OV7725_H
#define __OV7725_H 

#include <stdint.h>

#define CAMERA_W            80             //定义摄像头图像宽度
#define CAMERA_H            60				//定义摄像头图像高度
#define CAMERA_INTERLACE    1              	//摄像头间隔采集行数 n - 1,这里1表示不隔行采集，2表示隔行采集

#define CAMERA_DMA_NUM      (CAMERA_W / 8 )    //DMA采集次数
#define CAMERA_SIZE         (CAMERA_W * CAMERA_H / 8)        //图像占用空间大小


#define CAMERA_DMA_LINE     (CAMERA_H/CAMERA_INTERLACE)     //实际采集行数


extern   uint8_t *	    IMG_BUFF;       //图像缓冲区指针         

//定义图像采集状态
typedef enum 
{
    IMG_NOTINIT=0,
	IMG_FINISH,			//图像采集完毕
	IMG_FAIL,				//图像采集失败(采集行数少了)
	IMG_GATHER,				//图像采集中
	IMG_START,				//开始采集图像
	IMG_STOP,				//禁止图像采集
	
}IMG_STATE;

typedef struct
{
	uint8_t Address;			       /*寄存器地址*/
	uint8_t Value;		           /*寄存器值*/
}Register_Info;

extern 	uint8_t Ov7725_vsync;

#define camera_vsync()          ov7725_eagle_vsync()
#define camera_href()           //ov7725_eagle_href()
#define camera_dma()            ov7725_eagle_dma()

extern uint8_t Ov7725_Init(uint8_t *imgaddr);
extern void    Ov7725_exti_Init(void);
extern void    ov7725_eagle_vsync(void);
extern void    ov7725_eagle_dma(void);
extern void    ov7725_get_img(void);

extern	int  	OV7725_ReadReg(uint8_t LCD_Reg,uint16_t LCD_RegValue);
extern	int  	OV7725_WriteReg(uint8_t LCD_Reg,uint16_t LCD_RegValue);

//#define	ARRAY_INDEX(array)		((uint16_t)(sizeof(array)/sizeof(array[0])))
//#define	OV7725_INIT(regcfg)		Ov7725_Init(((Register_Info *)(regcfg)),ARRAY_INDEX(regcfg))


#endif























