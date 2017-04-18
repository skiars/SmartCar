#include "includes.h"
#include "uart_debug.h"
#include "oled_i2c.h"
#include "os_timers.h"
#include "uart_cmd.h"
#include "OV7725.h"
#include "turn.h"
#include "key.h"

// 随便新建的一个任务, 主要用于运行一些测试的代码
void task(void * argument)
{
//    char data[] = { 0x0b, 0xBB };
    TrunController *turn;

    // 测试死区电压
    //printf("死区电压测试...\n");
    //BSP_MoterThresholdTest();
    turn = TrunController_Init(image_buff, CAMERA_W, CAMERA_H);
    turnControl = turn;
    while (1) {
        ov7725_get_img();
        TrunController_Iteration(turn); // 方向环迭代
        debug_disp_image(turn->track);

//        uart_senddata(data, 2);
//        uart_senddata((const char *)image_buff, 600);
        if (
#if 0
            READ_BITS(turn->track->flags, ON_CROSS_WAY) ||
#endif
            READ_BITS(turn->track->flags, ON_LOOP_WAY)) {
            BSP_BuzzerSwitch(1);
        } else {
            BSP_BuzzerSwitch(0);
        }
    }
}

// 按键和遥控任务
void key_task(void * argument)
{   
    while (1) {
        BSP_RemoteProcess();
        delay_ms(20);
    }
}

// 启动任务, 在启动任务中初始化硬件, 而不是在main函数中,
// 因为此时操作系统提供的服务以及你给可以使用, 例如延时函数,
// 如果在main函数中使用FreeRTOS的延时功能会卡死.
void startTask(void * argument)
{
    uint32_t interval = 500, count; // 默认闪烁频率为1Hz
    board_init(); // MCU外设和板载外设初始化
    // 创建任务, 注意堆栈大小是否足够!!!!
    xTaskCreate(task, "task", 200U, NULL, 1, NULL);
    xTaskCreate(cmd_task, "cmd task", 400U, NULL, 0, NULL);
    xTaskCreate(key_task, "key task", 200U, NULL, 0, NULL);
    Create_Timers(); // 创建需要的软件定时器
    
    // LED闪烁, 程序运行指示灯
    SIM_HAL_EnableClock(SIM, kSimClockGatePortB);
    PORT_HAL_SetMuxMode(PORTB, 19, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 19, kGpioDigitalOutput);

    while (1) {
        GPIO_HAL_WritePinOutput(PTB, 19, 1);
        delay_ms(interval);
        GPIO_HAL_WritePinOutput(PTB, 19, 0);
        delay_ms(interval);
        
        // 一但电池电压很低就快速闪烁报警, 并且不恢复正常的频率.
        if (interval == 500) {
            // 7.0V是指带负载时的电池电压, 此时电池空载电压大约为7.2V.
            if (BSP_GetBatteryVoltage() < 7.0f) {
                if (count++ >= 5) { // 连续5次电压低于7.0V, 也就是5s
                    interval = 50; // 10Hz闪烁频率
                }
            } else {
                count = 0;
            }
        }
    }
}

// 主函数
int main(void)
{
    xTaskCreate(startTask, "startTask", 200U, NULL, 0, NULL);
    // 启动调度器，任务开始执行
    vTaskStartScheduler();
    // 正常情况下不会运行到这里
    while (1);
}
