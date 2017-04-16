#include "turn.h"
#include "moter.h"
#include "FreeRTOS.h"
#include "sendwave.h"
#include "uart_debug.h"

// 方向环控制器
TrunController *turnControl = NULL;

// 转向控制器初始化
TrunController *TrunController_Init(uint8_t *image, int width, int height)
{
    uint8_t *bitmap = pvPortMalloc(width * height * sizeof(int16_t));
    TrunController *obj = pvPortMalloc(sizeof(TrunController));

    PID_Init(&obj->pid, 0.4f, 0.0f, 1.2f, 1000);
    obj->track = TrackInfo_Init(bitmap, width, height);
    obj->image = image;
    obj->forward = 10; // 默认前瞻行数
    return obj;
}

// 摄像头图片解压
static void bitmap_decode(uint8_t *dst, uint8_t *src, size_t size)
{
    uint8_t t;
    int i;
    
    size >>= 3;
    for (i = 0; i < size; ++i) {
        t = ~*src++;
        *dst++ = t & (1 << 7) ? 0xFF : 0x00;
        *dst++ = t & (1 << 6) ? 0xFF : 0x00;
        *dst++ = t & (1 << 5) ? 0xFF : 0x00;
        *dst++ = t & (1 << 4) ? 0xFF : 0x00;
        *dst++ = t & (1 << 3) ? 0xFF : 0x00;
        *dst++ = t & (1 << 2) ? 0xFF : 0x00;
        *dst++ = t & (1 << 1) ? 0xFF : 0x00;
        *dst++ = t & (1 << 0) ? 0xFF : 0x00;
    }
}

// 获取方向偏差
static int _getTrunOffset(TrunController *ctrl)
{
    int k;
    TrackInfo *info = ctrl->track;
    
    Track_GetOffset(info);
    k = 0; //info->slope / 20;
    return info->offset + info->slope * (int)(ctrl->forward + abs_int(k)); // 计算拟合偏差
}

// 转向控制器迭代
void TrunController_Iteration(TrunController *obj)
{
    int control;
    
    bitmap_decode(obj->track->image, obj->image, obj->track->width * obj->track->height);
    Track_GetEdgeLine(obj->track);
    control = PID_Iteration(&obj->pid, -_getTrunOffset(obj));
    Moter_SetAngle(&moterControl, control);

#if 0
    {
        char buf[83];
        int n = max_int(obj->track->endLine[LEFT], obj->track->endLine[RIGHT]) * 100;
        
        ws_frame_init(buf);
        ws_add_int32(buf, CH1, -offset);
        ws_add_int32(buf, CH2, obj->track->offset);
        ws_add_int32(buf, CH3, obj->track->slope);
        ws_add_int32(buf, CH4, n);
        uart_senddata(buf, ws_frame_length(buf));
    }
#endif
}
