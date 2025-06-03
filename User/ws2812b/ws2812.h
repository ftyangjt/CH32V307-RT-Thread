#ifndef __WS2812_H
#define __WS2812_H

#include "ch32v30x.h"
#include <rtthread.h>

// 配置参数
#define WS2812_LED_NUM       30      // LED灯数量
#define WS2812_PWM_FREQ      800000  // PWM频率800KHz
#define WS2812_BIT_0_DUTY    33      // 0码占空比33%
#define WS2812_BIT_1_DUTY    66      // 1码占空比66%

/* HSV结构体 */
typedef struct {
    float h;  // 色相 0~360度
    float s;  // 饱和度 0~1
    float v;  // 亮度 0~1
} ws2812_hsv_t;

// 初始化WS2812 
void ws2812_init(void);

// 发送颜色数据到WS2812
void ws2812_update(uint8_t *rgb_data);

// HSV转RGB函数
void ws2812_hsv_to_rgb(ws2812_hsv_t hsv, uint8_t* r, uint8_t* g, uint8_t* b);

#endif /* __WS2812_H */