/********************************** (C) COPYRIGHT *******************************
* File Name          : screen.h
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/06/27
* Description        : 屏幕显示模块头文件
*********************************************************************************/

#ifndef __SCREEN_H
#define __SCREEN_H

#define DARK_BLUE 0x21CA   // #233851 的RGB565表示
#define LIGHT_BG 0xFFBE   // #F9F5F0 的RGB565表示
#define BG_TOP    0x21CA   // 深蓝
#define BG_BOTTOM 0x4F7D   // 浅蓝
#define FEEDING_TOP    0x1965   // 深海蓝 #1B4B73 
#define FEEDING_BOTTOM 0x0821   // 海底蓝 #082142


// 提取RGB565分量
#define RGB565_R(c) (((c) >> 11) & 0x1F)
#define RGB565_G(c) (((c) >> 5) & 0x3F)
#define RGB565_B(c) ((c) & 0x1F)

// 合成RGB565
#define RGB565(r,g,b) (((r)<<11)|((g)<<5)|(b))

#include "ch32v30x.h"
#include <rtthread.h>
#include "BSP/LCD/lcd.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./MALLOC/malloc.h"
#include "./TEXT/text.h"
#include "./PICTURE/piclib.h"
#include "./BSP/LED/led.h"
#include "string.h"
#include "stdio.h"
#include "./BSP/ADC/temp_adc.h"
#include <math.h>

// 函数声明
void pic_show_thread_entry(void *parameter);
rt_thread_t screen_init(void);
void display_aquarium_ui(void);
void update_time_display(int hour, int min, int sec);
void update_temperature_display(void);
#endif /* __SCREEN_H */
