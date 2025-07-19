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
#define BG_TOP    0x4E7F
#define BG_BOTTOM 0x2D3B

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
