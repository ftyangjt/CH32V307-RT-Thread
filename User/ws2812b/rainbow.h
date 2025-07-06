#ifndef __RAINBOW_H
#define __RAINBOW_H

#include "ch32v30x.h"
#include <rtthread.h>

// 彩虹效果控制结构体
typedef struct {
    float hue;           // 当前基础色相
    float hue_step;      // 色相步长
    uint16_t delay_ms;   // 更新延时(ms)
    uint8_t brightness;  // 亮度(0-100)
    uint8_t mode;        // 模式(0:停止, 1:旋转, 2:呼吸)
    rt_bool_t running;   // 运行状态
} rainbow_ctrl_t;

// 初始化彩虹效果
void rainbow_init(void);

// 启动彩虹效果
void rainbow_start(uint8_t speed_level);

// 停止彩虹效果
void rainbow_stop(void);

// 设置彩虹效果速度
void rainbow_set_speed(uint8_t speed_level);

// 设置彩虹效果亮度
void rainbow_set_brightness(uint8_t brightness);

// 停止白色灯光效果
void white_mode_stop(void);

#endif /* __RAINBOW_H */