#include "pid.h"

// PID初始化
void PID_Init(PID *pid, float p, float i, float d, int scale)
{
    pid->p = (int)(p * scale);
    pid->i = (int)(i * scale);
    pid->d = (int)(d * scale);
    pid->scale = scale;
    pid->sum = 0;
    pid->error = 0;
    pid->value = 0;
}

// PID设置设定值
void PID_SetValue(PID *pid, int value)
{
    pid->value = value;
}

// PID设置参数(浮点)
void PID_SetParams(PID *pid, float p, float i, float d)
{
    pid->p = (int)(p * pid->scale);
    pid->i = (int)(i * pid->scale);
    pid->d = (int)(d * pid->scale);
}

// PID迭代计算
int PID_Iteration(PID *pid, int measured)
{
    int error, derror;
    
    error = pid->value - measured;     // 计算本次测量误差
    derror = error - pid->error;       // 误差微分
    pid->sum += error;                 // 计算误差积分
    pid->error = error;                // 更新误差项
    /* 计算输出值 */
    return (pid->p * error              // 比例项
        + pid->i * pid->sum             // 积分项
        + pid->d * derror)              // 微分项
        / pid->scale;                   // 整点计算
}
