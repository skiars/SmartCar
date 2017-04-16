#ifndef __MOTER_H
#define __MOTER_H

#include "pid.h"

typedef struct {
    int angle;          // 转向角度差速值
    PID standPID;       // 直立环PID
    PID speedPID;       // 两个车轮的速度控制
    int standOut;       // 直立环输出
    int speedOut;       // 速度环输出
} MoterControl;

extern MoterControl moterControl;

// 幅度限制(钳位)
static inline int clamp_int(int value, int range)
{
    return value > range ? range : (value < -range ? -range : value);
}

// 幅度限制(钳位) float版本
static inline float clamp_float(float value, float range)
{
    return value > range ? range : (value < -range ? -range : value);
}

void Moter_ControlInit(MoterControl *moter);
void Moter_SetSpeed(MoterControl *moter, int speed);
void Moter_SetAngle(MoterControl *moter, int angle);
void Moter_StandIteration(MoterControl *moter);
void Moter_SpeedIteration(MoterControl *moter);
void Moter_Output(MoterControl *moter);

#endif
