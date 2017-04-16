#include "uart_cmd.h"
#include "fsl_uart_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "mystring.h"

// 命令最多支持的参数数量
#define CMD_MAX_PARAM_COUNT  20

// 定义消息队列
static xQueueHandle cmdQueue;

// 读取一个字符
static int cmd_getchar(char *rxbuf, int rxpos, char ch)
{
    if (ch == '#') { // '#'表示强制解析后面的命令并丢弃前面的类容
        rxpos = 0;
        rxbuf[0] = '\0';
    } else if (ch == '\n' || ch == ';' || ch == '\r') { // 命令结束符
        rxbuf[rxpos] = '\0';
        rxpos = 0;
    } else {
        rxbuf[rxpos++] = ch;
    }
    return rxpos;
}

// 解析命令
static void analyze_cmd(char *lineBuf)
{
    int argc = 0, len;
    char *argv[CMD_MAX_PARAM_COUNT];
    const CommandProcess *tab = UartCmd_Tab;
    
    mystrlwr(lineBuf); // 转换成小写
    while (*lineBuf && argc < (CMD_MAX_PARAM_COUNT - 1)) {
        char *p, *param;
        
        JUMP_SPACE(lineBuf); // 跳过无效字符
        if (*lineBuf == '\0') {
            break;
        }
        // 计算第一个单词的长度
        p = lineBuf;
        while (*p && *p != ' ' && *p != '\t') { // 空格和制表符是分隔符
            ++p;
        }
        len = (size_t)p - (size_t)lineBuf + 1;
        param = pvPortMalloc(len);
        strncpy(param, lineBuf, len - 1); // 复制参数内容
        param[len - 1] = '\0';
        argv[argc++] = param;
        lineBuf = p; // 到下一个参数
    }
    // 在命令列表中匹配合适的命令
    while (tab->name && strcmp(tab->name, argv[0])) {
        ++tab;
    }
    // 调用命令回调函数来处理命令
    if (tab->func) {
        tab->func(argc, argv);
    } else {
        printf("Can not found the command: \"%s\".\n", argv[0]);
    }
    // 释放空间
    while (argc--) {
        vPortFree(argv[argc]);
    }
}

// 命令操作任务
void cmd_task(void * argument)
{
    int count = 0;
    static char lineBuf[1000]; // 指令缓冲, 定义成静态数组可以减少堆栈消耗
    
    cmdQueue = xQueueCreate(100, 1); // 创建队列
    while (1) {
        char ch;
        
        // 当队列不为空时读取数据
        xQueueReceive(cmdQueue, &ch, portMAX_DELAY);
        count = cmd_getchar(lineBuf, count, ch);
        if (count == 0 && *lineBuf) { // 接收到一行命令
            printf(">%s\n", lineBuf);
            // 解析命令
            analyze_cmd(lineBuf);
        }
    }
}

// 串口中断服务函数
void UART1_RX_TX_IRQHandler(void)
{
    if (UART_HAL_GetStatusFlag(UART1, kUartRxDataRegFull)) {
        char ch = UART1->D;
        
        xQueueSendToBack(cmdQueue, &ch, 0); // 向队列中加入数据
        UART_HAL_ClearStatusFlag(UART1, kUartRxDataRegFull);
    }
}
