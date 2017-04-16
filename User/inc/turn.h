#ifndef __TURN_H
#define __TURN_H

#include <stdint.h>

#include "pid.h"
#include "image_process.h"

typedef struct {
    PID pid;            // 转向环PID控制器
    TrackInfo *track;   // 赛道信息
    uint8_t *image;     // 摄像头位图指针
    int forward;        // 前瞻行数
} TrunController;

extern TrunController *turnControl;

TrunController *TrunController_Init(uint8_t *image, int width, int height);
void TrunController_Iteration(TrunController *obj);

#endif
