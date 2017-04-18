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

// 判断圆环的方向
static void _getLoopDir(TrackInfo * info)
{
    static int count = 0;

    // 只有在圆环中蔡判断圆环的方向
    if (READ_BITS(info->flags, ON_LOOP_WAY)) {
        // 圆环方向标志无效
        if (!READ_BITS(info->flags, LOOPDIR_VALID)) {
            int y, end, width = info->width - 1, l, r;
            int16_t *edge;

            // 识别左边的出口
            end = info->endLine[LEFT];
            edge = info->edge[LEFT];
            for (y = 1; y < end; ++y) {
                // 检测到前方出口
                if (edge[y] == 0 && edge[y] - edge[y - 1] < -5) {
                    l = y;
                    break;
                }
            }
            // 识别右边的出口
            end = info->endLine[RIGHT];
            edge = info->edge[RIGHT];
            for (y = 1; y < end; ++y) {
                // 检测到前方出口
                if (edge[y] == width && edge[y] - edge[y - 1] > 5) {
                    r = y;
                    break;
                }
            }
            info->loopDir = l < r ? LEFT : RIGHT;
            SET_BITS(info->flags, LOOPDIR_VALID); // 圆环方向有效标志置位
            count = 0; // 复位计时器
        }
    } else {
        if (count < 20) {
            ++count; // 清标志计时
        } else { // 如果计数达到10次则圆环方向有效标志清零
            CLEAR_BITS(info->flags, LOOPDIR_VALID);
        }
    }
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
        if (d1 < -3 && d1 > -14 && d2 > 1 && d2 < 4) {
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
        if (d1 > 3 && d1 < 14 && d2 < -1 && d2 > -4) {
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
                if (i - l > 10) {
                    return l;
                }
            }
        }
    }
    return 0;
}

// 圆环打角
void _loopRun(TrackInfo * info)
{
    int i, line, dir, y0;
    static int status = 0;

    y0 = min_int(info->endLine[LEFT], info->endLine[RIGHT]);
    for (dir = 0; dir < 2; ++dir) {
        line = info->endLine[dir] - 4;
        // 寻找拐点行
        for (i = 5; i < line; ++i) {
            // 判断是否是拐点
            if ((info->edge[dir][i] - info->edge[dir][i - 2])
                * (info->edge[dir][i - 2] - info->edge[dir][i - 5]) < -4) {
                y0 = min_int(y0, i); // 记录拐点行数
                break;
            }
        }
    }
    if (y0 < 16) {
        SET_BITS(info->flags, LOOP_TURN);
        switch (status) {
            case 0:
                status = 1; // 进路口
                break;
            case 2:
                status = 3; // 出路口
                break;
        default:
                break;
        }
    } else if (y0 > 25) {
        CLEAR_BITS(info->flags, LOOP_TURN);
        switch (status) {
            case 1:
                status = 2; // 已经进路口
                break;
            case 3:
                if (y0 > 40) {
                    status = 0; // 已经出路口
                    CLEAR_BITS(info->flags, ON_LOOP_WAY);
                }
                break;
        default:
                break;
        }
    }
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
        if (READ_BITS(info->flags, ON_LOOP_WAY) || _fingLoop(info)) { // 检查是否在圆环中
            SET_BITS(info->flags, ON_LOOP_WAY);
            _getLoopDir(info); // 获取圆环出口方向
            _loopRun(info);
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
        if (READ_BITS(info->flags, ON_LOOP_WAY)) {
            _loopRun(info);
        }
    }
}

// 获取赛道偏差
int Track_GetOffset(TrackInfo *info)
{
    int meanX = 0, meanY = 0, sum1 = 0, sum2 = 0, n, i;
    int middle = info->width >> 1;

    // 求需要拟合的点数
    n = max_int(info->endLine[LEFT], info->endLine[RIGHT]);
    n = min_int(n - 5, 45); // 最多采用前45行数据, 并且, 在丢线前的几行数据可能很不稳定, 不使用
    if (n <= 5) { // 样本点数少于5点时直接返回
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
