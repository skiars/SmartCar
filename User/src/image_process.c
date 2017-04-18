#include "image_process.h"
#include <stdio.h>

#ifdef __ARMCC_VERSION
#include "oled_i2c.h"
#include "FreeRTOS.h"
#define t_malloc(size)    pvPortMalloc(size)
#define t_free(p)         vPortFree(p)
#else
#include <malloc.h>
#define t_malloc(size)    malloc(size)
#define t_free(p)         free(p)
#endif

// 创建一个赛道信息结构
TrackInfo *TrackInfo_Init(uint8_t *image, int width, int height)
{
    TrackInfo *info = t_malloc(sizeof(TrackInfo));

    info->edge[LEFT] = t_malloc(height * sizeof(int16_t));
    info->edge[RIGHT] = t_malloc(height * sizeof(int16_t));
    info->middle = t_malloc(height * sizeof(int16_t));
    info->image = image;
    info->width = width;
    info->height = height;
    info->flags = 0x00;
    info->loopDir = LEFT;
    info->count = 0;
    return info;
}

// 释放TrackInfo空间
void TrackInfo_Free(TrackInfo * info)
{
    t_free(info->edge[LEFT]);
    t_free(info->edge[RIGHT]);
    t_free(info->middle);
    t_free(info);
}

// 扫描前面几行数据
static short scanFront(TrackInfo * info)
{
    char flag = 0; // 边界丢失标志,赛道宽度已经设置标志
    uint8_t *pImg;
    short middle = info->width / 2; // 图像中点
    short x, y;

    info->endLine[LEFT] = 0;
    info->endLine[RIGHT] = 0;
    // 扫描整行
    for (y = 0; y < FRONT_LINE; ++y) {
        pImg = info->image + (info->height - 1 - y) * info->width;
        flag = 0x00;
        // 寻找左边界
        for (x = middle; x >= 0; --x) {
            if (pImg[x] == 0x00) { // 黑色
                info->edge[LEFT][y] = x + 1; // 记录边缘
                break;
            }
        }
        if (x < 0) { // 左边界丢失
            flag |= 0x01;
        }
        // 寻找右边界
        for (x = middle; x < info->width; ++x) {
            if (pImg[x] == 0x00) { // 黑色
                info->edge[RIGHT][y] = x - 1; // 记录边缘
                break;
            }
        }
        if (x >= info->width) { // 右边界丢失
            flag |= 0x02;
        }
        if (flag) { // 边界丢失
            if (flag & 0x01) { // 左边界丢失
                info->edge[LEFT][y] = 0;
                info->endLine[RIGHT] = y;
            }
            if (flag & 0x02) { // 右边界丢失
                info->edge[RIGHT][y] = info->width - 1;
                info->endLine[LEFT] = y;
            }
            if (flag == 0x03) { // 两边赛道都丢失
                // 在返回之前先把下面几行数据进行赋值, 以免出现不确定的情况
                for (x = y; x < FRONT_LINE; ++x) {
                    info->edge[LEFT][x] = 0;
                    info->edge[RIGHT][x] = info->width - 1;
                    info->middle[x] = middle;
                }
                return y;
            }
        } else {
            info->endLine[LEFT] = y;
            info->endLine[RIGHT] = y;
        }
        // 计算中线
        info->middle[y] = (info->edge[LEFT][y] + info->edge[RIGHT][y]) / 2;
    }
    return y;
}

// 扫描剩下的列
int scanNext(TrackInfo * info, short y)
{
    char flag = 0; // 赛道丢失标志, 提取结束标志
    short x, pos;
    int width = info->width;
    uint8_t *line1, *line2, color, dir;

    line2 = info->image + (int)(info->height - y) * width;
    // 后面的列搜索扫描
    for (; y < info->height; ++y) {
        line1 = line2;
        line2 = info->image + (int)(info->height - 1 - y) * width;
        flag &= ~0x03; // 丢线标志清零
        for (dir = 0; dir < 2; ++dir) {
            x = info->edge[dir][y - 1]; // 上一行边沿坐标是下一行的开始搜索位置
            x = min_int(max_int(x, 0), width - 1); // 钳位, 该语句应当不起作用, 只是为了保险
            if ((x == 0 || x == width - 1) && line2[x] == 0xFF) { // 图像边沿是白色时直接标记丢线
                info->edge[dir][y] = x;
            } else { // 否则开始扫描赛道边线
                if (x == 0) { // 边界在最左边时只向右搜索
                    pos = 1;
                } else if (x == width - 1) { // 边界在最右边时只向左搜索
                    pos = -1;
                } else { // 上一行x左边的值和当前行x值相同时向右搜索, 否则向左搜索
                    pos = line1[x - 1] == line2[x] ? 1 : -1;
                }
                color = line2[x] ? 0x00 : 0xFF; // 当前行x点颜色为白色则匹配目标为黑色, 否则为白色
                for (; x >= 0 && x < info->width; x += pos) {
                    if (line2[x] == color) { // 匹配颜色时标记
                        // 扫描匹配颜色是白色时直接保存x值, 是黑色时则要保存 x - pos
                        x -= color == 0xFF ? 0 : pos;
                        info->edge[dir][y] = x;
                        break;
                    }
                }
                // 满足下列情况时表示边界丢失，需要补线
                if (x < 0 || x >= info->width || abs_int(x - info->edge[dir][y - 1]) > 10) {
                    flag |= 1 << dir;
                } else {
                    flag &= ~(1 << (dir + 2)); // 清除赛道行数记录标志
                }
            }
        }
        if (flag) { // 需要补线
            if (flag & 0x01) { // 左边需要补线
                info->edge[LEFT][y] = 0;
                if (!(flag & 0x04)) { // 如果之前没有标记过丢线则记录赛道行数
                    info->endLine[LEFT] = y;
                    flag |= 0x04;
                }
            }
            if (flag & 0x02) { // 右边需要补线
                info->edge[RIGHT][y] = info->width - 1;
                if (!(flag & 0x08)) {  // 如果之前没有标记过丢线则记录赛道行数
                    info->endLine[RIGHT] = y;
                    flag |= 0x08;
                }
            }
            if ((flag & 0x03) == 0x03) { // 两边都丢线
                return y;
            }
        }
        // 计算中线
        info->middle[y] = (info->edge[LEFT][y] + info->edge[RIGHT][y]) / 2;
    }
    info->endLine[LEFT] = y;
    info->endLine[RIGHT] = y;
    return y;
}

/* 在十字路口里跳过空白的赛道 */
static int scanJumpWhite(TrackInfo * info, int y)
{
    uint8_t flag = 0, *img;
    short x, endline, i, dx, dy;
    short width = info->width; /* 图像宽度 */
    short middle = width / 2; /* 图像中点 */

    // 记录开始丢线的行
    endline = y;
    /* 跳过没有边界的赛道(十字路口中横着的赛道),
       for循环直到扫描完整个图像或者赛道两边均找到边线时结束 */
    for (; y < info->height; ++y) {
        img = info->image + (int)(info->height - 1 - y) * width;
        flag = 0x00;
        for (x = middle; x >= 0 && img[x]; --x); /* 寻找左边界 */
        if (x >= 0) { // 发现左边界标志
            // 当边线变化速度小于6像素每行时记录
            if (y > 1 && INT_ABS(x - info->edge[LEFT][y - 1]) < 6) {
                flag |= 0x01;
            }
            info->edge[LEFT][y] = x;
        } else {
            info->edge[LEFT][y] = 0;
        }
        for (x = middle; x < width && img[x]; ++x); /* 寻找右边界 */
        if (x < width) { /* 发现右边界标志 */
            // 当边线变化速度小于6像素每行时记录
            if (y > 1 && INT_ABS(x - info->edge[RIGHT][y - 1]) < 6) {
                flag |= 0x02;
            }
            info->edge[RIGHT][y] = x;
        } else {
            info->edge[RIGHT][y] = width - 1;
        }
        if (flag == 0x03) { // 两边中线都找到了
            break;
        }
    }
    info->middle[y] = (info->edge[LEFT][y] + info->edge[RIGHT][y]) / 2;

    // 补画中线
    if (endline == 0) {
        info->middle[0] = middle;
    } else if (endline >= 3) {
        endline = endline > 3 ? endline - 3 : 0;
    }
    x = info->middle[endline];
    dx = info->middle[y] - info->middle[endline];
    dy = y - endline;
    for (i = endline; i < y; ++i) {
        info->middle[i] = x + (i - endline) * dx / dy;
    }
    return y + 1;
}

// 识别十字路口
static int _findCros(TrackInfo * info)
{
    int out, i, endl, endr;

    out = max_int(info->endLine[LEFT], info->endLine[RIGHT]);
    // 识别直入十字路口
    if (out > 10 || READ_BITS(info->flags, ON_CROSS_WAY)) {
        endl = 0;
        endr = 0;
        out = min_int(out, 30);
        for (i = 0; i < out; ++i) {
            uint8_t flag = 0x00;

            if (info->edge[LEFT][i] == 0) {
                flag |= 0x01;
                if (!endl) { // 记录左边开始丢线行数
                    endl = i;
                }
            }
            if (info->edge[RIGHT][i] == info->width - 1) {
                flag |= 0x02;
                if (!endr) { // 记录右边开始丢线行数
                    endr = i;
                }
            }
            // 两边同时丢线则返回首先丢线的行
            if (flag == 0x03) {
                return min_int(endl, endr);
            }
        }
    }
    // 十字路口标志已经置位
    if (READ_BITS(info->flags, ON_CROSS_WAY)) {
        // 如果不丢线行数大于30行表示十字路口结束
        if (max_int(info->endLine[LEFT], info->endLine[RIGHT]) > 30) {
            CLEAR_BITS(info->flags, ON_CROSS_WAY); // 清除十字路口标志
        } else { // 否则返回开始丢线行
            return min_int(info->endLine[LEFT], info->endLine[RIGHT]);
        }
    }
    return 0;
}

// 判断前方是否是圆环, 检查赛道是否有连续变宽的趋势
static short _fingLoop(TrackInfo * info)
{
    int16_t *edge;
    int i, out, l = 0, r = 0, d1, d2;

    // 检测左边的拐角
    out = info->endLine[LEFT];
    edge = info->edge[LEFT];
    for (i = 6; i < out; ++i) {
        d1 = edge[i] - edge[i - 3];
        d2 = edge[i - 3] - edge[i - 6];
        // 拐角处前面几行向左延伸, 后面几行向右延伸
        if (d1 < -3 && d1 > -20 && d2 > 1 && d2 < 4) {
            l = i - 3;
            break;
        }
    }

    // 检测右边的拐角
    out = info->endLine[RIGHT];
    edge = info->edge[RIGHT];
    for (i = 6; i < out; ++i) {
        d1 = edge[i] - edge[i - 3];
        d2 = edge[i - 3] - edge[i - 6];
        // 拐角处前面几行向右延伸, 后面几行向左延伸
        if (d1 > 3 && d1 < 20 && d2 < -1 && d2 > -4) {
            r = i - 3;
            break;
        }
    }

    // 两边都存在拐角
    if (l && r) {
        int middle, height = info->height, width = info->width;

        // 寻找前方的圆环
        l = min_int(l, r);
        out = min_int(l + 20, info->height);
        middle = info->middle[l];
        for (i = l; i < out; ++i) {
            uint8_t *img = info->image + (int)(height - 1 - i) * width;
            if (!img[middle]) { // 遇到黑色
                if (i - l > 6) {
                    return l;
                }
            }
        }
    }
    return 0;
}

// 圆环补线
static short _loopGetLine(TrackInfo * info)
{
    int i, line, dir, out;
    int y0, x0, y1, dx, dy;
    int width = info->width, height = info->height;
    uint8_t *img = info->image + (height - 1) * width;

    for (dir = 0, y0 = info->height; dir < 2; ++dir) {
        line = info->endLine[dir] - 4;
        // 寻找拐点行
        for (i = 5; i < line; ++i) {
            // 判断是否是拐点
            if ((info->edge[dir][i] - info->edge[dir][i - 2])
                * (info->edge[dir][i - 2] - info->edge[dir][i - 5]) < -4) {
                break;
            }
        }
        y0 = min_int(y0, i); // 记录拐点行数
    }
    dir = info->loopDir;
    // 寻找丢线行
    out = min_int(info->endLine[LEFT], info->endLine[RIGHT]);
    for (line = 0; line < out; ++line) {
        if (info->edge[LEFT][line] <= 0
            || info->edge[RIGHT][line] >= width - 1) {
            break;
        }
    }
    if (y0 >= 30 && line > 40) { // 说明已经出圆环
        if (info->slope < 40) {
            return 0;
        }
    }

//    printf("line: %d, y0: %d, middle: %d\n", line, y0, info->middle[y0]);
    // 如果没有拐点就说明附近是路口, 此时边线会丢线, 将扫描的x坐标设置为width / 2,
    // 否则说明前方一段距离才是路口, 以拐点行的中线作为扫描x坐标
    x0 = y0 < line ? info->middle[y0] : info->middle[line];
    // 向前寻找, 直到不再丢线(遇到黑色)
    for (i = y0; i < height - 1; ++i) {
        img = info->image + (height - 1 - i) * width;
        if (img[x0] == 0x00) { // 前方为黑色时跳出
            break;
        }
    }
    y1 = i; // 记录扫描行数

    // 没有找到拐点行说明附近就是路口
    if (y0 >= line) {
        for (i = 0, out = 0; i < 10; ++i) {
            out += info->edge[RIGHT][i] - info->edge[LEFT][i];
        }
        out /= 10; // 赛道平均宽度
//        printf("line: %d\n", out);
        if (out < 65) {
            return -1;
        }
        y0 = line; // 没有拐点, 所以指定y0为开始丢线的行
        // 寻找边界: 遇到白色像素结束搜索
        if (dir == LEFT) { // 当出口在左边时
            for (i = 0; i < width && img[i] == 0x00; ++i); // 寻找左边的边线
            info->edge[LEFT][y1] = i;
            for (; i < width && img[i]; ++i); // 寻找右边边线
            info->edge[RIGHT][y1] = i;
        } else { // 当出口在右边时
            for (i = width; i > 0 && img[i] == 0x00; ++i); // 寻找右边的边线
            info->edge[LEFT][y1] = i;
            for (; i > 0 && img[i]; ++i); // 寻找左边的边线
            info->edge[RIGHT][y1] = i;
        }
    } else { // 附近找到拐点说明前方是环形路口
        // 寻找前方赛道(一般是内圈的圆环)
        if (dir == LEFT) { // 如果路口向左拐就寻找右边线
            for (i = x0; i > 0 && !img[i]; --i); // 直到图像变为白色
            info->edge[RIGHT][y1] = i; // 右边线
            for (; i > 0 && img[i]; --i); // 直到黑色
            info->edge[LEFT][y1] = i; // 右边线
        } else { // 否则寻找左边线
            for (i = x0; i < width && !img[i]; ++i); // 直到图像变为白色
            info->edge[LEFT][y1] = i;
        }
        --y0;
    }

    info->middle[y1] = (info->edge[LEFT][y1] + info->edge[RIGHT][y1]) / 2;
    // 补画中线
    x0 = info->middle[y0];
    dx = info->middle[y1] - x0;
    dy = y1 - y0;
    for (i = y0; i < y1; ++i) {
        info->middle[i] = x0 + (i - y0) * dx / dy;
    }
    return y1 + 1;
}

// 提取赛道边线
void Track_GetEdgeLine(TrackInfo * info)
{
    int y, width = info->width, height = info->height;

    info->endLine[LEFT] = 0;
    info->endLine[RIGHT] = 0;
    // 赛道边线初始化
    for (y = 0; y < height; ++y) {
        info->edge[LEFT][y] = 0;
        info->edge[RIGHT][y] = width - 1;
    }
    y = scanFront(info);
    if (y == FRONT_LINE) {
        y = scanNext(info, FRONT_LINE); // 扫描剩下的赛道
        if (_fingLoop(info) || READ_BITS(info->flags, ON_LOOP_WAY)) { // 检查是否在圆环中
            y = _loopGetLine(info);
            if (y > 0) {
                scanNext(info, y);
                SET_BITS(info->flags, ON_LOOP_WAY); // 设置圆环路口标志
                CLEAR_BITS(info->flags, ON_CROSS_WAY); // 清除十字路口标志
            } else if (y == 0) {
                CLEAR_BITS(info->flags, ON_LOOP_WAY);
            }
        } else { // 如果不在圆环中就判断是不是在十字路口
            y = _findCros(info);
            if (y) { // 在十字路口
                y = scanJumpWhite(info, y);
                scanNext(info, y);
                SET_BITS(info->flags, ON_CROSS_WAY); // 设置十字路口标志
            }
        }
    } else {
        if (READ_BITS(info->flags, ON_CROSS_WAY)) { // 在十字路口中
            y = scanJumpWhite(info, y);
            scanNext(info, y);
        }
        if (READ_BITS(info->flags, ON_LOOP_WAY)) { // 在圆环中
            y = _loopGetLine(info);
            if (y) {
                scanNext(info, y);
            }
        }
    }
}

// 最小二乘法拟合附近赛道的斜率和偏差
int Track_GetOffset(TrackInfo *info)
{
    int meanX = 0, meanY = 0, sum1 = 0, sum2 = 0, n, i;
    int middle = info->width >> 1;

    // 求需要拟合的点数
    n = max_int(info->endLine[LEFT], info->endLine[RIGHT]);
    n = min_int(n - 5, 45); // 最多采用前45行数据, 并且, 在丢线前的几行数据可能很不稳定, 不使用
    if (n <= 5) { // 样本点数少于5点时直接返回, 并使用上一次的值
        return info->offset;
    }
    // 求x, y平均值
    // 求sigma(x * y)和sigma(x * x)
    for (i = 0; i < n; ++i) {
        meanX += i;
        meanY += (info->middle[i] - middle);
        sum1 += i * (info->middle[i] - middle);
        sum2 += i * i;
    }
    meanX /= n;
    meanY /= n;

    if ((sum2 - n * meanX * meanX) != 0) { // 判断除数是否为0
        // 求斜率, 斜率放大100倍
        info->slope = (sum1 - n * meanX * meanY) * 100 / (sum2 - n * meanX * meanX);
        // 求偏差, 偏差放大100倍
        info->offset = meanY * 100 - info->slope * meanX;
    }
    return info->offset;
}

// 计算赛道曲率
int Track_GetCurvity(TrackInfo *info)
{
    // 获取赛道有效行数

    return 0;
}
