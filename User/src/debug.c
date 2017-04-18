#include "uart_cmd.h"
#include "includes.h"
#include "moter.h"
#include "turn.h"

static int cmd_reset(int argc, char *argv[]);
static int cmd_help(int argc, char *argv[]);
static int cmd_pid(int argc, char *argv[]);
static int cmd_speed(int argc, char *argv[]);
static int cmd_stop(int argc, char *argv[]);
static int cmd_forward(int argc, char *argv[]);
static int cmd_battery(int argc, char *argv[]);
static int cmd_start(int argc, char *argv[]);

const CommandProcess UartCmd_Tab[] = {
    { "reset",      cmd_reset },
    { "help",       cmd_help },
    { "pid",        cmd_pid },
    { "speed",      cmd_speed },
    { "stop",       cmd_stop },
    { "star",       cmd_stop }, // 紧急停车指令, start删除t形成
    { "forward",    cmd_forward },
    { "battery",    cmd_battery },
    { "start",      cmd_start },
    { NULL,         NULL }
};

static int cmd_reset(int argc, char *argv[])
{
    printf("\nReset...\n");
    NVIC_SystemReset();
    return 0;
}

static int cmd_help(int argc, char *argv[])
{
    const CommandProcess *tab = UartCmd_Tab;
    
    printf("NXP智能车摄像头直立类 - Creeper队.\n"
        );
    printf("指令列表:\n");
    while (tab->name) {
        printf("    %s\n", tab->name);
        ++tab;
    }
    return 0;
}

// PID参数设置命令
static int cmd_pid(int argc, char *argv[])
{
    PID *pid;

    if (argc == 1) { // 显示PID参数信息
        pid = &moterControl.standPID;
        printf("直立环 P: %f, I: %f, D: %f.\n",
            (float)pid->p / pid->scale,
            (float)pid->i / pid->scale,
            (float)pid->d / pid->scale);
        pid = &moterControl.speedPID;
        printf("速度环 P: %f, I: %f, D: %f.\n",
            (float)pid->p / pid->scale,
            (float)pid->i / pid->scale,
            (float)pid->d / pid->scale);
        pid = &turnControl->pid;
        printf("方向环 P: %f, I: %f, D: %f.\n",
            (float)pid->p / pid->scale,
            (float)pid->i / pid->scale,
            (float)pid->d / pid->scale);
    } else if (argc == 5) { // 设置PID参数
        float p = atof(argv[2]);
        float i = atof(argv[3]);
        float d = atof(argv[4]);

        if (!strcmp(argv[1], "stand")) { // 直立环
            pid = &moterControl.standPID;
        } else if (!strcmp(argv[1], "speed")) { // 速度环
            pid = &moterControl.speedPID;
        } else if (!strcmp(argv[1], "turn")) { // 方向环
            pid = &turnControl->pid;
        } else {
            printf("error: 不存在PID控制器 \"%s\"\n", argv[1]);
            return 1;
        }
        PID_SetParams(pid, p, i, d);
    } else {
        printf("error: 命令格式错误. eg:\n"
               "  pid [stand/speed/turn] (float)P (float)I (float)D.\n");
        return 1;
    }
    return 0;
}

// 设置车速
static int cmd_speed(int argc, char *argv[])
{
    if (argc == 1) { // 显示速度
        printf("小车当前速度: %d\n", moterControl.speedPID.value);
    } else if (argc == 2) { // 设置速度
        PID_SetValue(&moterControl.speedPID, atoi(argv[1]));
    } else {
        printf("error: 命令格式错误. eg:\n"
               "  speed: value.\n");
        return 1;
    }
    return 0;
}

// 紧急停车
static int cmd_stop(int argc, char *argv[])
{
    BSP_MoterDisable();
    printf("Moter driver is disabled.\n");
    return 0;
}

// 设置前瞻行数
static int cmd_forward(int argc, char *argv[])
{
    if (turnControl == NULL) {
        printf("方向控制器尚未初始化!\n");
        return 1;
    }
    if (argc == 1) { // 显示前瞻行数
        printf("前瞻行数: %d\n", turnControl->forward);
    } else { // 设置前瞻行数
        turnControl->forward = atoi(argv[1]);
    }
    return 0;
}

static int cmd_battery(int argc, char *argv[])
{
    printf("\tVoltage: %.2fV.\n", BSP_GetBatteryVoltage());
    return 0;
}

static int cmd_start(int argc, char *argv[])
{
    BSP_MoterEnable();
    printf("Moter driver is enabled.\n");
    return 0;
}
