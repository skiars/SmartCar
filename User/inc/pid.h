#ifndef __PID_H
#define __PID_H

typedef struct {
    int p, i, d;        // PID参数
    int value;          // 设定值
    int error;          // 上次误差
    int sum;            // 误差积分值
    int scale;          // 整形对浮点的缩放因子
} PID;

void PID_Init(PID *pid, float p, float i, float d, int scale);
void PID_SetValue(PID *pid, int value);
void PID_SetParams(PID *pid, float p, float i, float d);
int PID_Iteration(PID *pid, int measured);

#endif
