#include "kalman_filter.h"

//卡尔曼噪声参数
#define dt           0.005f
#define R_angle      10.0f     // R值表示测量噪声协方差, R越小收敛越快, R过大或过小都会使滤波效果变差
#define Q_angle      0.01f     // 角度数据置信度
#define Q_gyro       0.0001f   // 角加速度置信度

static float angle, angle_dot;//角度和角速度
static float P[2][2] = { { 1, 0 }, { 0, 1 } };
static float Pdot[4] = { 0, 0, 0, 0 };
static float C_0 = 1;
static float q_bias, angle_err, PCt_0, PCt_1, E, K_0, K_1, t_0, t_1;

//卡尔曼滤波
float Kalman_Filter(float angle_m, float gyro_m, float *gryo)//angleAx 和 gyroGy
{
    angle += (gyro_m - q_bias) * dt;
    angle_err = angle_m - angle; // 测量值 - 预测值
    Pdot[0] = Q_angle - P[0][1] - P[1][0];
    Pdot[1] = -P[1][1];
    Pdot[2] = -P[1][1];
    Pdot[3] = Q_gyro;
    P[0][0] += Pdot[0] * dt;
    P[0][1] += Pdot[1] * dt;
    P[1][0] += Pdot[2] * dt;
    P[1][1] += Pdot[3] * dt;
    PCt_0 = C_0 * P[0][0];
    PCt_1 = C_0 * P[1][0];
    E = R_angle + C_0 * PCt_0;
    K_0 = PCt_0 / E; // 卡尔曼增益
    K_1 = PCt_1 / E;
    t_0 = PCt_0;
    t_1 = C_0 * P[0][1];
    P[0][0] -= K_0 * t_0;
    P[0][1] -= K_0 * t_1;
    P[1][0] -= K_1 * t_0;
    P[1][1] -= K_1 * t_1;
    angle += K_0 * angle_err; // 最优角度 = 预测值 + 卡尔曼增益 * (测量值 - 预测值)
    q_bias += K_1 * angle_err;
    angle_dot = gyro_m - q_bias; // 最优角速度

    *gryo = angle_dot;
    return angle;
}
