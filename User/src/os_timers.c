#include "os_timers.h"
#include "FreeRTOS.h"
#include "timers.h"
#include <stdio.h>
#include "moter.h"

// 电机控制定时器
void Moter_Timer(TimerHandle_t xTimer)
{
    // 直立环迭代
    Moter_StandIteration(&moterControl);
    // 速度环迭代
    Moter_SpeedIteration(&moterControl);
    Moter_Output(&moterControl);
}

void Create_Timers(void)
{
    TimerHandle_t timer;
    
    // 电机控制单元初始化
    Moter_ControlInit(&moterControl);
    
    // 创建定时器
    timer = xTimerCreate("Moter Timer", 5, pdTRUE, 0, Moter_Timer);
    xTimerStart(timer, 0);
}
