/********************************** (C) COPYRIGHT *******************************
* File Name          : rainbow.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/05/27
* Description        : WS2812彩虹灯光效果
*********************************************************************************/

#include "rainbow.h"
#include "ws2812.h"
#include <math.h>

// 全局变量
static rainbow_ctrl_t rainbow_ctrl;
static rt_thread_t rainbow_thread = RT_NULL;
static uint8_t led_data[WS2812_LED_NUM * 3];

/* 彩虹效果线程 */
static void rainbow_thread_entry(void* parameter)
{
    ws2812_hsv_t hsv;
    hsv.s = 1.0f;
    float hue = 0.0f;
    rt_kprintf("rainbow init\n");
    while(1) {
        if(!rainbow_ctrl.running) {
            rt_thread_mdelay(100);  // 不运行时降低CPU负担
            continue;
        }
        
        // 设置亮度(0-100转换为0.0-1.0)
        hsv.v = (float)rainbow_ctrl.brightness / 100.0f;
        
        // 更新所有LED颜色
        for(int i = 0; i < WS2812_LED_NUM; i++) {
            hsv.h = fmodf(hue + (float)i * (360.0f / WS2812_LED_NUM), 360.0f);
            
            ws2812_hsv_to_rgb(hsv, 
                &led_data[i * 3 + 0],
                &led_data[i * 3 + 1],
                &led_data[i * 3 + 2]);
        }
        
        // 更新WS2812
        ws2812_update(led_data);
        
        // 更新色相
        hue += rainbow_ctrl.hue_step;
        if(hue >= 360.0f) hue -= 360.0f;
        
        // 根据当前设定的延时参数延时
        rt_thread_mdelay(rainbow_ctrl.delay_ms);
    }
}

/* 初始化彩虹效果 */
void rainbow_init(void)
{
    // 初始化控制参数
    rainbow_ctrl.hue = 0.0f;
    rainbow_ctrl.hue_step = 5.0f;
    rainbow_ctrl.delay_ms = 25;
    rainbow_ctrl.brightness = 30;
    rainbow_ctrl.mode = 1;
    rainbow_ctrl.running = RT_FALSE;
    
    // 创建彩虹效果线程
    rainbow_thread = rt_thread_create("rainbow", 
                                       rainbow_thread_entry, 
                                       RT_NULL, 
                                       512, 
                                       10, 
                                       10);
    if(rainbow_thread != RT_NULL) {
        rt_thread_startup(rainbow_thread);
        rt_kprintf("彩虹效果线程已初始化\n");
    }else {
    rt_kprintf("彩虹效果线程初始化失败！！！\n");
    }
}

/* 启动彩虹效果 */
void rainbow_start(uint8_t speed_level)
{
    if(speed_level < 1) speed_level = 1;
    if(speed_level > 5) speed_level = 5;
    
    rainbow_ctrl.hue_step = 2.0f * speed_level;
    rainbow_ctrl.delay_ms = 50 / speed_level;
    rainbow_ctrl.running = RT_TRUE;
    
    rt_kprintf("彩虹效果已启动，速度级别=%d\n", speed_level);
}

/* 停止彩虹效果 */
void rainbow_stop(void)
{
    rainbow_ctrl.running = RT_FALSE;
    rt_kprintf("彩虹效果已停止\n");
}

/* 设置彩虹效果速度 */
void rainbow_set_speed(uint8_t speed_level)
{
    if(speed_level < 1) speed_level = 1;
    if(speed_level > 5) speed_level = 5;
    
    rainbow_ctrl.hue_step = 2.0f * speed_level;
    rainbow_ctrl.delay_ms = 50 / speed_level;
    
    rt_kprintf("彩虹效果速度已设置为级别%d\n", speed_level);
}

/* 设置彩虹效果亮度 */
void rainbow_set_brightness(uint8_t brightness)
{
    if(brightness > 100) brightness = 100;
    rainbow_ctrl.brightness = brightness;
    
    rt_kprintf("彩虹效果亮度已设置为%d%%\n", brightness);
}