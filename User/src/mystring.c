#include "mystring.h"
#include <stdio.h>

/* 将字符串中的大写字母转换为小写 */
void mystrlwr(char *s)
{
    char c;

    while ((c = *s) != 0) {
        if (c >= 'A' && c <= 'Z') {
            *s += 32;
        }
        ++s;
    }
}
