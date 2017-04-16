#ifndef __IMAGE_PROCESS_H
#define __IMAGE_PROCESS_H

#include <stdint.h>

/* 定义起始行 */
#define FRONT_LINE     4

/* 整数绝对值计算 */
#define INT_ABS(X) ((X) > 0 ? (X) : -(X))

/* 计算最小值 */
#define INT_MIN(X, Y) ((X) < (Y) ? (X) : (Y))

/* 计算最大值 */
#define INT_MAX(X, Y) ((X) > (Y) ? (X) : (Y))

/* 检查x坐标是否超出边界 */
#define CHECK_EDGE(X, W) ((X) < (W) ? ((X) > 0 ? (X) : 0) : (W) - 1)

// 十字路口标志
#define ON_CROSS_WAY    0x02
// 在圆环中标志
#define ON_LOOP_WAY     0x04

#define SET_BITS(a, b)      ((a) |= (b))
#define CLEAR_BITS(a, b)    ((a) &= ~(b))
#define READ_BITS(a, b)     (((a) & (b)) == b)

#ifndef LEFT
  #define LEFT                0u
#endif

#ifndef RIGHT
  #define RIGHT               1u
#endif

typedef struct {
    int16_t *edge[2];       // 左右两边的边线
    int16_t *middle;        // 中线
    int16_t endLine[2];      // 结束行
    uint8_t *image;         // 位图缓冲
    int width, height;      // 位图尺寸
    int curvity;            // 赛道曲率
    int slope;              // 赛道斜率
    int offset;             // 偏差
    uint8_t flags;          // 标志
    uint8_t loopDir;        // 圆环方向
    int count;
} TrackInfo;

// 计算最大值
static inline int max_int(int x, int y)
{
    return x > y ? x : y;
}

// 计算最小值
static inline int min_int(int x, int y)
{
    return x < y ? x : y;
}

// 计算绝对值
static inline int abs_int(int x)
{
    return x > 0 ? x : -x;
}

TrackInfo *TrackInfo_Init(uint8_t *image, int width, int height);
void TrackInfo_Free(TrackInfo * info);
void Track_GetEdgeLine(TrackInfo * info);
int Track_GetOffset(TrackInfo *info);
int Track_GetCurvity(TrackInfo *info);


#endif
