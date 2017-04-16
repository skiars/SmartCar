#include "moter.h"
#include <math.h>
#include "bsp.h"
#include "mpu6050.h"
#include "sendwave.h"
#include "uart_debug.h"
#include "kalman_filter.h"
#include "oled_i2c.h"
#include <stdio.h>
#include "FreeRTOS.h"

// 波形显示模式设置
// 0: 不显示
// 1: 串口示波器
// 2: 串口输出陀螺仪零飘(1000次采样)
// 3: 显示速度环波形
#define DISP_WAVE 0

// 输出模式, 非调试时应该设置为0
// 0: 默认模式(直立环, 速度环, 方向环)
// 1: 只有直立环
// 2: 只有直立环和速度环
#define OUTPUT_MODE 0

// 陀螺仪零飘, 单位dps
#define GYRO_OFFSET 1.17f

// 左轮死区电压PWM值
#define MOTER_LEFT_THRESHOLD   100u

// 右轮死区电压PWM值
#define MOTER_RIGHT_THRESHOLD  100u

// 直立时车的角度(加速计陀螺仪测量值)
#define SENSOR_STAND_ANGLE     12.0f

// 安全角度
#define SPEED_SAFE_ANGLE       4.0f

// 电机控制器
MoterControl moterControl;

// 电机控制环初始化
void Moter_ControlInit(MoterControl *moter)
{
    moter->angle = 0;
    // 直立环
    PID_Init(&moter->standPID, 0.4f, 0.0f, 0.05f, 1000);
    PID_Init(&moter->speedPID, 0.0f, 0.01f, 0.0f, 10);
    PID_SetValue(&moter->standPID, 0); // 直立环设定值
    PID_SetValue(&moter->speedPID, 0); // 速度环设定值
    moter->standOut = 0;
    moter->speedOut = 0;
}

// 设置电机转速
void Moter_SetSpeed(MoterControl *moter, int speed)
{
    moter->speedPID.value = speed;
}

// 设置转角
void Moter_SetAngle(MoterControl *moter, int angle)
{
    taskENTER_CRITICAL();
    moter->angle = angle;
    taskEXIT_CRITICAL();
}

// 直立环迭代
void Moter_StandIteration(MoterControl *moter)
{
    int err;
    int16_t data[3];
    float angle;
    static float angle_y, gyro_y;     //用static修饰，因为这两个参数是随时都在变的，而且每次变得时候的前一次的值是要保证保持前一次的

    // 获取陀螺仪和加速计的读数
    err = mpu6050_read_accel(data);
    if (!err) { // 没有错误时赋值数据
        angle_y = -atan2f(data[0], data[2]) * 180.0f / 3.1415926535f;
    }
    err = mpu6050_read_gyro(data);
    if (!err) { // 没有错误时赋值数据
        gyro_y = (data[1] / 16.38f - GYRO_OFFSET); // 陀螺仪零飘补偿
    }

    // Kalman滤波器融合参数
    angle = Kalman_Filter(angle_y, gyro_y, &gyro_y) - SENSOR_STAND_ANGLE;
    // 直立PD控制
    moter->standOut =  (int)(moter->standPID.p * angle + moter->standPID.d * gyro_y / 22.2f);

#if DISP_WAVE == 1 // 显示波形
    {
        char buf[83];

        ws_frame_init(buf);
        ws_add_float(buf, CH1, angle_y);
        ws_add_float(buf, CH2, gyro_y);
        ws_add_float(buf, CH3, angle);
        ws_add_float(buf, CH4, moter->standOut);
        uart_senddata(buf, ws_frame_length(buf));
    }
#elif DISP_WAVE == 2 // 测量陀螺仪零飘
    {
        static int gyro_count;
        static float gryo_sum;
        
        if (gyro_count++ < 1000) {
            gryo_sum += gyro_y;
        } else {
            printf("加速计读数平均值: %f\n", gryo_sum / 1000.0f);
            gyro_count = 0;
            gryo_sum = 0;
        }
    }
#endif
}

// 速度环迭代
void Moter_SpeedIteration(MoterControl *moter)
{
    static float outNew, outOld, integral; // 新的速度输出值, 旧的速度输出值, 误差积分
    static int speed, count = 0; // 速度累加值, 速度环累积计数器
    float trueSpeed, fetw; // 实际速度值, 比例项

    // 测量编码器采集到的速度, 每次测量都采集, 防止编码器饱和
    speed += BSP_GetEncoderCount(LEFT) + BSP_GetEncoderCount(RIGHT);
    // 速度平滑滤波
    if (count++ >= 19) { // 速度环在直立环调节20个周期后进行调节
        int error;
        float kp = (float)moter->speedPID.p / moter->speedPID.scale;
        float ki = (float)moter->speedPID.i / moter->speedPID.scale;

        // 实际速度的计算原理: 当输出PWM值为1000时100ms间隔下单个编码器采集值大约为6490,
        // 则换算关系: trueSpeed = speed / (6490.0f * 2.0f / 1000.0f) = speed / 12.98f.
        trueSpeed = speed / 12.98f;
        error = moter->speedPID.value - trueSpeed;   // 计算本次误差
        if (error < 200) { // 积分分离, 如果误差大于正200时积分项不起作用
            integral += error * ki; // 误差积分值
            integral = clamp_float(integral, 4000); // 积分限幅
        } else {
            integral = 0;
        }
        fetw = clamp_float(kp * error, 2000); // 比例限幅
        outOld = outNew;                 // 更新旧的速度控制值
        outNew = fetw + integral;  // PI控制
        outNew = clamp_float(outNew, 3000.0f); // 限幅
        count = 0;
        speed = 0; // 重新累计编码器读数
    }
    // 分20次输出
    moter->speedOut = (int)((outNew - outOld) * count / 20.0f + outOld);
#if DISP_WAVE == 3 // 速度环调试显示波形
    {
        char buf[83];

        ws_frame_init(buf);
        ws_add_float(buf, CH1, trueSpeed);
        ws_add_int32(buf, CH2, moter->speedOut);
        ws_add_float(buf, CH3, integral);
        ws_add_float(buf, CH4, fetw);
        uart_senddata(buf, ws_frame_length(buf));
    }
#endif
}

// 电机控制输出
void Moter_Output(MoterControl *moter)
{
    static int turnAngle;
    int speed, sum, diff;

#if OUTPUT_MODE == 0 || OUTPUT_MODE == 2
    // 计算直立环和速度环的输出和
    sum = moter->standOut - moter->speedOut;
#else // 只有直立环的调试模式
    sum = moter->standOut;
#endif

#if OUTPUT_MODE == 0
    taskENTER_CRITICAL();
    turnAngle = (moter->angle * 6 + turnAngle * 4) / 10; // 速度滤波
    taskEXIT_CRITICAL();
    diff = turnAngle; // 角度转换为两轮差速
#else // 调试模式没有方向环
    diff = 0;
#endif

    // 左轮控制
    speed = sum + diff;
    // 死区电压补偿
    speed += speed >= 0 ? MOTER_LEFT_THRESHOLD : -MOTER_LEFT_THRESHOLD;
    BSP_SetMotorSpeed(LEFT, clamp_int(speed, 3500)); // 输出限幅

    // 右轮控制
    speed = sum - diff;
    // 死区电压补偿
    speed += speed >= 0 ? MOTER_RIGHT_THRESHOLD : -MOTER_LEFT_THRESHOLD;
    BSP_SetMotorSpeed(RIGHT, clamp_int(speed, 3500)); // 输出限幅
}
