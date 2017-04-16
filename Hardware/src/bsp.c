#include "bsp.h"
#include "includes.h"
#include "uart_debug.h"
#include "mpu6050.h"
#include "fsl_adc16_hal.h"
#include "OLED_I2C.h"
#include <OLED_I2C.h>

uint8_t image_buff[CAMERA_SIZE];

// FTM3初始化
static void FTM3_Init(void)
{
    ftm_pwm_param_t ftmParam = {
        .mode                   = kFtmEdgeAlignedPWM,
        .edgeMode               = kFtmHighTrue,
    };

    // FTM3时钟使能
    SIM_HAL_EnableClock(SIM, kSimClockGateFtm3);
    FTM_HAL_Init(FTM3);
    FTM_HAL_SetSyncMode(FTM3, kFtmUseSoftwareTrig); // 软件触发
    FTM_HAL_SetClockSource(FTM3, kClock_source_FTM_SystemClk); // FTM时钟源

    // PWM初始化
    /* Clear the overflow flag */
    FTM_HAL_ClearTimerOverflow(FTM3);

    // 左边电机控制
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 0);
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 1);

    // 右边电机控制
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 2);
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 3);
    FTM_HAL_SetMod(FTM3, MOTER_PWM_RANGE - 1);

    // PWM占空比初始化为 0%
    FTM_HAL_SetChnCountVal(FTM3, 0, 0);
    FTM_HAL_SetChnCountVal(FTM3, 1, 0);
    FTM_HAL_SetChnCountVal(FTM3, 2, 0);
    FTM_HAL_SetChnCountVal(FTM3, 3, 0);
    FTM_HAL_SetClockPs(FTM3, kFtmDividedBy1); // 分频
    FTM_HAL_SetSoftwareTriggerCmd(FTM3, true); // 触发更新
}

// 电机PWM初始化
static void motor_init(void)
{
    // 电机使用FTM3
    SIM_HAL_EnableClock(SIM, kSimClockGatePortD);

    // PWM输出引脚的端口复用为 FTM3_CHn
    PORT_HAL_SetMuxMode(PORTD, 0, kPortMuxAlt4);
    PORT_HAL_SetMuxMode(PORTD, 1, kPortMuxAlt4);
    PORT_HAL_SetMuxMode(PORTD, 2, kPortMuxAlt4);
    PORT_HAL_SetMuxMode(PORTD, 3, kPortMuxAlt4);

    // 电机驱动模块的片选信号
    PORT_HAL_SetMuxMode(PORTD, 4, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTD, 4, kGpioDigitalOutput);
    GPIO_HAL_WritePinOutput(PTD, 4, 1); // BTN7971高电平使能
    
    FTM3_Init();
}

// 电机控制器使能
void BSP_MoterEnable(void)
{
    GPIO_HAL_WritePinOutput(PTD, 4, 1);
}

// 电机控制器失能
void BSP_MoterDisable(void)
{
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
}

// 正交编码器初始化
void FTM_QuadPhaseEncodeInit(FTM_Type *ftmBase)
{
    FTM_HAL_SetQuadMode(ftmBase, kFtmQuadPhaseEncode);
    FTM_HAL_SetQuadPhaseAFilterCmd(ftmBase, true);

    /* Set Phase A filter value if phase filter is enabled */
    FTM_HAL_SetChnInputCaptureFilter(ftmBase, CHAN0_IDX, 10);
    FTM_HAL_SetQuadPhaseBFilterCmd(ftmBase, true);

    /* Set Phase B filter value if phase filter is enabled */
    FTM_HAL_SetChnInputCaptureFilter(ftmBase, CHAN1_IDX, 10);
    FTM_HAL_SetQuadPhaseAPolarity(ftmBase, kFtmQuadPhaseNormal);
    FTM_HAL_SetQuadPhaseBPolarity(ftmBase, kFtmQuadPhaseNormal);

    FTM_HAL_SetQuadDecoderCmd(ftmBase, true);
    FTM_HAL_SetMod(ftmBase, 0xFFFF); // 此操作没有什么意义

    /* Set clock source to start the counter */
    FTM_HAL_SetClockSource(ftmBase, kClock_source_FTM_SystemClk);
}

// 正交编码器初始化
static void RotaryEncoder_Init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    // 左边编码器
    PORT_HAL_SetMuxMode(PORTA, 10, kPortMuxAlt6);
    PORT_HAL_SetMuxMode(PORTA, 11, kPortMuxAlt6);
    // 右边编码器
    PORT_HAL_SetMuxMode(PORTA, 12, kPortMuxAlt7);
    PORT_HAL_SetMuxMode(PORTA, 13, kPortMuxAlt7);

    // 开启FTM时钟
    SIM_HAL_EnableClock(SIM, kSimClockGateFtm1);
    SIM_HAL_EnableClock(SIM, kSimClockGateFtm2);

    FTM_QuadPhaseEncodeInit(FTM1);
    FTM_QuadPhaseEncodeInit(FTM2);
}

// 遥控初始化
static void remote_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    PORT_HAL_SetMuxMode(PORTA, 15u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTA, 15u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTA, 15u, kPortPullDown); // 下拉
}

// 蜂鸣器初始化
static void buzzer_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    PORT_HAL_SetMuxMode(PORTA, 14, kPortMuxAsGpio);
    PORT_HAL_SetDriveStrengthMode(PORTA, 14, kPortHighDriveStrength);
    GPIO_HAL_SetPinDir(PTA, 14, kGpioDigitalOutput);
}

// ADC初始化(用于测量电池电压)
void adc_init(void)
{
    adc16_converter_config_t config = {
        .lowPowerEnable = true,
        .clkDividerMode = kAdc16ClkDividerOf8,
        .longSampleTimeEnable = 8,
        .resolution = kAdc16ResolutionBitOfSingleEndAs12, // 12bit
        .clkSrc = kAdc16ClkSrcOfAsynClk,
        .asyncClkEnable = true,
        .highSpeedEnable = false,
        .longSampleCycleMode = kAdc16LongSampleCycleOf24,
        .hwTriggerEnable = false,
        .refVoltSrc = kAdc16RefVoltSrcOfVref,
        .continuousConvEnable = false,
        .dmaEnable = false
    };

    // Enable clock for ADC.
    SIM_HAL_EnableClock(SIM, kSimClockGateAdc0);
    ADC16_HAL_Init(ADC0);
    ADC16_HAL_ConfigConverter(ADC0, &config);
}

// 按键初始化
static void key_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortB);

    PORT_HAL_SetMuxMode(PORTB, 20u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 20u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 20u, kPortPullUp); // 上拉
    PORT_HAL_SetMuxMode(PORTB, 21u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 21u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 21u, kPortPullUp); // 上拉
    PORT_HAL_SetMuxMode(PORTB, 22u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 22u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 22u, kPortPullUp); // 上拉
    PORT_HAL_SetMuxMode(PORTB, 23u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 23u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 23u, kPortPullUp); // 上拉
    
}

// 硬件初始化
void board_init(void)
{
    uart_debug_init(115200);
    key_init();
    adc_init();
    motor_init();
    RotaryEncoder_Init();
    mpu6050_init();
    Ov7725_Init(image_buff);
    OLED_Init();
    remote_init();
    buzzer_init();
}

// 获取总线频率
uint32_t BSP_GetBusClockFreq(void)
{
    return SystemCoreClock / (CLOCK_HAL_GetOutDiv2(SIM) + 1);
}

// 获取旋转解码器的数值
// left: 1:左边编码器, 0: 右边编码器
int BSP_GetEncoderCount(int left)
{
    int count;
    FTM_Type *ftmBase = left == LEFT ? FTM2 : FTM1;

    count = (int16_t)FTM_HAL_GetCounter(ftmBase); 
    FTM_HAL_SetCounter(ftmBase, 0);
    if (left == RIGHT) { // 右轮方向相反
        count = -count;
    }
    return count;
}

// 设置电机转速
void BSP_SetMotorSpeed(int left, int value)
{
    uint16_t pwm1, pwm2;

    if (value > 0) {
        pwm1 = value;
        pwm2 = 0;
    } else {
        pwm1 = 0;
        pwm2 = -value;
    }
    if (left == LEFT) { // 左轮
        FTM_HAL_SetChnCountVal(FTM3, 0, pwm2);
        FTM_HAL_SetChnCountVal(FTM3, 1, pwm1);
    } else { // 右轮
        FTM_HAL_SetChnCountVal(FTM3, 2, pwm1);
        FTM_HAL_SetChnCountVal(FTM3, 3, pwm2);
    }
    FTM_HAL_SetSoftwareTriggerCmd(FTM3, true); // 触发更新
}

// 测量电机死区电压(PWM值)测试
void BSP_MoterThresholdTest(void)
{
    int i, speedCnt, cnt_L = 0, cnt_R = 0;

    // PWM值慢慢加大
    for (i = 0; i < MOTER_PWM_RANGE / 10; ++i) {
        BSP_SetMotorSpeed(LEFT, i);
        BSP_SetMotorSpeed(RIGHT, i);
        // 左轮死区电压测量
        speedCnt = BSP_GetEncoderCount(LEFT);
        if (speedCnt > 10 && cnt_L++ < 20) { // 过滤编码器小幅度的抖动, 最多输出20次值
            printf("Left moter PWM: %d, speed count: %d\n", i, speedCnt);
        }
        // 右轮死区电压测量
        speedCnt = BSP_GetEncoderCount(RIGHT);
        if (speedCnt > 10 && cnt_R++ < 200) { // 过滤编码器小幅度的抖动, 最多输出20次值
            printf("Right moter PWM: %d, speed count: %d\n", i, speedCnt);
        }
        delay_ms(100);
    }
}

// 测试电机转速和编码器速度的关系
void BSP_MoterSpeedTest(void)
{
    BSP_SetMotorSpeed(LEFT, 1000);
    BSP_SetMotorSpeed(RIGHT, 1000);
    while (1) {
        delay_ms(100);
        printf("PWM: 1000, time: 100ms left speed: %d, right speed: %d\n",
            BSP_GetEncoderCount(LEFT), BSP_GetEncoderCount(RIGHT));
    }
}

// 遥控处理
void BSP_RemoteProcess(void)
{
    /*
    if (GPIO_HAL_ReadPinInput(PTA, 15) && GPIO_HAL_ReadPinInput(PTD, 4)) {
        BSP_MoterDisable(); // 电机失能
        printf("Moter driver is disabled.\n");
    }
    */
}

// 蜂鸣器开关
void BSP_BuzzerSwitch(int8_t sw)
{
    if (sw) {
        GPIO_HAL_WritePinOutput(PTA, 14, 1);
    } else {
        GPIO_HAL_WritePinOutput(PTA, 14, 0);
    }
}

// 测量ADC值
float BSP_GetBatteryVoltage(void)
{
    int value;
    adc16_chn_config_t ch_cfg = {
        .chnIdx = kAdc16Chn23,
        .convCompletedIntEnable = false,
        .diffConvEnable = false
    };

    // Software trigger the conversion.
    ADC16_HAL_ConfigChn(ADC0, 0, &ch_cfg);
    // Wait for the conversion to be done.
    while (!ADC16_HAL_GetChnConvCompletedFlag(ADC0, 0));
    // Fetch the conversion value.
    value = ADC16_HAL_GetChnConvValue(ADC0, 0);
    return value / 4095.0f * 3.3f * 3.0f;
}

// 获取按键电平
uint8_t BSP_GetKeyValue(uint8_t key)
{
    switch (key) {
        case KEY1:
            key = 20;
            break;
        case KEY2:
            key = 21;
            break;
        case KEY3:
            key = 22;
            break;
        case KEY4:
            key = 23;
            break;
        default:
            return 0;
    }
    return GPIO_HAL_ReadPinInput(PTB, key) == 0;
}

