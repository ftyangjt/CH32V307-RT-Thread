#ifndef __RAINBOW_H
#define __RAINBOW_H

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#include "rtthread.h"
#include "ws2812.h"
#include <stdint.h>


// 彩虹灯效果控制结构体
typedef struct {
    float hue;                  // 当前色调值(0-360)
    float hue_step;             // 色调变化步长
    uint8_t delay_ms;           // 更新间隔(毫秒)
    uint8_t brightness;         // 亮度(0-100)
    uint8_t mode;               // 模式选择
    uint8_t breathing_enabled;  // 是否启用呼吸效果
    uint32_t breathing_cycle_ms; // 呼吸周期(毫秒)
    uint8_t min_brightness;     // 最小亮度百分比(0-100)
} rainbow_ctrl_t;

/* 彩虹效果相关函数 */
void rainbow_init(void);
void rainbow_start(uint8_t speed_level);
void rainbow_stop(void);
void rainbow_set_speed(uint8_t speed_level);
void rainbow_set_brightness(uint8_t brightness);

/* 纯色模式相关函数 */
void solid_color_start(void);
void solid_color_stop(void);
void solid_color_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void solid_color_set_brightness(uint8_t brightness);

/* 白光模式函数（向后兼容） */
void white_mode_start(void);
void white_mode_stop(void);

void breathing_start(uint32_t cycle_time_s);
void breathing_stop(void);
void breathing_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t max_brightness);

void rainbow_breathing_start(uint8_t speed_level, uint32_t breathing_cycle_s);
void rainbow_set_min_brightness(uint8_t min_brightness);
#endif /* __RAINBOW_H */