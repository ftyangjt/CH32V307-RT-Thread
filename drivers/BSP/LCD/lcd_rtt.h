#ifndef __LCD_RTT_H
#define __LCD_RTT_H

#include <rtthread.h>
#include <rtdevice.h>
#include "./BSP/LCD/lcd.h"

/* LCD设备结构体 */
struct lcd_device {
    struct rt_device parent;     /* RT-Thread设备父类 */
    rt_uint16_t width;           /* 屏幕宽度 */
    rt_uint16_t height;          /* 屏幕高度 */
    rt_uint8_t dir;              /* 屏幕方向 */
    rt_uint8_t id;               /* 屏幕ID */
    void *fb;                    /* 帧缓冲区 (如果有的话) */
};

/* LCD控制命令 */
#define RTGRAPHIC_CTRL_RECT_UPDATE      (128 + 0)    /* 更新指定区域 */
#define RTGRAPHIC_CTRL_POWERON          (128 + 1)    /* 打开LCD显示 */
#define RTGRAPHIC_CTRL_POWEROFF         (128 + 2)    /* 关闭LCD显示 */
#define RTGRAPHIC_CTRL_GET_INFO         (128 + 3)    /* 获取LCD信息 */
#define RTGRAPHIC_CTRL_SET_MODE         (128 + 4)    /* 设置LCD模式 */
#define RTGRAPHIC_CTRL_GET_EXT          (128 + 5)    /* 获取扩展信息 */
#define RTGRAPHIC_CTRL_SET_BRIGHTNESS   (128 + 6)    /* 设置亮度 */
#define RTGRAPHIC_CTRL_GET_BRIGHTNESS   (128 + 7)    /* 获取亮度 */
#define RTGRAPHIC_CTRL_SET_DIRECTION    (128 + 8)    /* 设置方向 */
#define RTGRAPHIC_CTRL_GET_DIRECTION    (128 + 9)    /* 获取方向 */

/* 初始化LCD的RT-Thread设备 */
int rt_hw_lcd_init(void);

#endif /* __LCD_RTT_H */