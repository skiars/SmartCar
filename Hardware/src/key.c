#include "key.h"
#include "includes.h"
#include "oled_i2c.h"

// 将摄像头采集到的数据变换成OLED可以使用的格式(OLED扫描方式比较特俗...)
void ImgConvent(uint8_t *Dst, uint8_t *Src, uint16_t Width, uint16_t Height)
{
    uint8_t t;
    uint16_t page, x, xByte = Width / 8;
    uint16_t i, j, k, m;

    for (j = 0; j < Height; ++j) {
        page = j / 8;
        m = j % 8;
        for (i = 0; i < xByte; ++i) {
            t = *Src++;
            for (k = 0; k < 8; ++k) {
                x = k + i * 8;
                if (!(t & 0x80)) {
                    Dst[page * Width + x] |= (1 << m); 
                } else {
                    Dst[page * Width + x] &= ~(1 << m); 
                }
                t <<= 1;
            }
        }
    }
}

// 将摄像头采集到的数据转换为OLED使用的格式
void Edge_OutputImage(uint8_t *buffer, TrackInfo *info)
{
    int i;
    short h, *line;
    uint8_t *p1, mod;

    for (i = 0; i < 80 * 64 / 8; ++i) {
        buffer[i] = 0xff;
    }
    h = INT_MAX(info->endLine[LEFT], info->endLine[RIGHT]);
    h = INT_MIN(h, CAMERA_H);
    for (i = 0; i < h; ++i) {
        /* 左边线 */
        line = info->edge[LEFT];
        if (line[i] < 0) {
            p1 = buffer + (CAMERA_H - 1 - i) * (CAMERA_W / 8);
            mod = 7;
        } else if (line[i] >= CAMERA_W) {
            p1 = buffer + (CAMERA_H - i) * (CAMERA_W / 8) - 1;
            mod = 0;
        } else {
            p1 = buffer + (CAMERA_H - 1 - i) * (CAMERA_W / 8) + (line[i] / 8);
            mod = 7 - line[i] % 8;
        }
        *p1 &= ~(1 << mod);
        /* 右边线 */
        line = info->edge[RIGHT];
        if (line[i] < 0) {
            p1 = buffer + (CAMERA_H - 1 - i) * (CAMERA_W / 8);
            mod = 7;
        } else if (line[i] >= CAMERA_W) {
            p1 = buffer + (CAMERA_H - i) * (CAMERA_W / 8) - 1;
            mod = 0;
        } else {
            p1 = buffer + (CAMERA_H - 1 - i) * (CAMERA_W / 8) + (line[i] / 8);
            mod = 7 - line[i] % 8;
        }
        *p1 &= ~(1 << mod);
        /* 中线 */
        line = info->middle;
        if (line[i] < 0) {
            p1 = buffer + (CAMERA_H - 1 - i) * (CAMERA_W / 8);
            mod = 7;
        } else if (line[i] >= CAMERA_W) {
            p1 = buffer + (CAMERA_H - i) * (CAMERA_W / 8) - 1;
            mod = 0;
        } else {
            p1 = buffer + (CAMERA_H - 1 - i) * (CAMERA_W / 8) + (line[i] / 8);
            mod = 7 - line[i] % 8;
        }
        *p1 &= ~(1 << mod);
    }
}

// 显示图像
void debug_disp_image(TrackInfo *info)
{
    static char mode = 0, key_status;
    static uint8_t buff[80 * 64], out[80 * 64];

    if (mode == 1) { // 显示边线
        Edge_OutputImage(buff, info);
        ImgConvent(out, buff, 80, 64);
        OLED_DrawBitmap(0, 0, 80, 8, out);
    } else if (mode == 2) { // 显示图像
        ImgConvent(out, image_buff, 80, 64);
        OLED_DrawBitmap(0, 0, 80, 8, out);
    }
    if (BSP_GetKeyValue(KEY1)) {
        key_status = 1;
    } else if (key_status) {
        key_status = 0;
        mode = mode < 2 ? mode + 1 : 0; // mode为0什么也不显示
    }
}
