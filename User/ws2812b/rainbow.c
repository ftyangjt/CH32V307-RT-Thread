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
static rt_thread_t white_thread = RT_NULL;
// 全局颜色参数，用于存储当前纯色模式的RGB值
static struct {
    uint8_t r;
    uint8_t g; 
    uint8_t b;
    uint8_t brightness;  // 亮度百分比(0-100)
} solid_color = {255, 255, 255, 100};  // 默认为白色，100%亮度

// 自定义字符串转整数函数
static int str_to_int(const char* str)
{
    int result = 0;
    int sign = 1;
    
    // 处理符号
    if(*str == '-') {
        sign = -1;
        str++;
    } else if(*str == '+') {
        str++;
    }
    
    // 处理数字
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
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
    
    rt_kprintf("彩虹效果已初始化\n");
}

/* 彩虹效果线程 */
static void rainbow_thread_entry(void* parameter)
{
    ws2812_hsv_t hsv;
    hsv.s = 1.0f;
    float hue = 0.0f;
    
    while(1) {
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

/* 启动彩虹效果 */
void rainbow_start(uint8_t speed_level)
{
    // 先停止白光模式
    white_mode_stop();
    
    // 如果彩虹线程已存在，先删除它
    if (rainbow_thread != RT_NULL)
    {
        rt_thread_delete(rainbow_thread);
        rainbow_thread = RT_NULL;
    }
    
    if(speed_level < 1) speed_level = 1;
    if(speed_level > 5) speed_level = 5;
    
    rainbow_ctrl.hue_step = 2.0f * speed_level;
    rainbow_ctrl.delay_ms = 50 / speed_level;
    
    // 创建彩虹效果线程
    rainbow_thread = rt_thread_create("rainbow", 
                                      rainbow_thread_entry, 
                                      RT_NULL, 
                                      512, 
                                      9, 
                                      10);
    if(rainbow_thread != RT_NULL) {
        rt_thread_startup(rainbow_thread);
        rt_kprintf("彩虹效果已启动，速度级别=%d\n", speed_level);
    } else {
        rt_kprintf("彩虹效果线程创建失败！\n");
    }
}

/* 停止彩虹效果 */
void rainbow_stop(void)
{
    if (rainbow_thread != RT_NULL)
    {
        rt_thread_delete(rainbow_thread);
        rainbow_thread = RT_NULL;
        
        // 关闭所有LED
        for (int i = 0; i < WS2812_LED_NUM * 3; i++)
        {
            led_data[i] = 0;
        }
        ws2812_update(led_data);
        
        rt_kprintf("彩虹模式已停止\n");
    }
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

// MSH命令：启动彩虹效果
static int cmd_rainbow(int argc, char **argv)
{
    int speed = 3;  // 默认速度级别
    
    if(argc > 1) {
        speed = str_to_int(argv[1]);
    }
    
    rainbow_start(speed);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow, start rainbow effect [speed 1-5]);

// MSH命令：停止彩虹效果
static int cmd_rainbow_stop(int argc, char **argv)
{
    rainbow_stop();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_stop, stop rainbow effect);

// MSH命令：设置彩虹效果亮度
static int cmd_brightness(int argc, char **argv)
{
    int brightness = 30;  // 默认亮度30%
    
    if(argc > 1) {
        brightness = str_to_int(argv[1]);
        if(brightness < 0) brightness = 0;
        if(brightness > 100) brightness = 100;
    }
    
    rainbow_set_brightness(brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_brightness, set rainbow brightness [0-100]);

/* 常亮白光模式线程 */
static void white_thread_entry(void* parameter)
{
    uint8_t white_data[WS2812_LED_NUM * 3];
    uint8_t brightness = 255; // 最大亮度

    while (1)
    {
        // 填充所有LED为白色，亮度可调
        for (int i = 0; i < WS2812_LED_NUM; i++)
        {
            white_data[i * 3 + 0] = brightness; // R
            white_data[i * 3 + 1] = brightness; // G
            white_data[i * 3 + 2] = brightness; // B
        }
        ws2812_update(white_data);
        rt_thread_mdelay(100); // 保持刷新
    }
}

/* 启动常亮白光模式 */

void white_mode_start(void)
{
    rainbow_stop(); // 先停止彩虹效果线程

    if (white_thread == RT_NULL)
    {
        white_thread = rt_thread_create("white_led",
                                        white_thread_entry,
                                        RT_NULL,
                                        512,
                                        9,
                                        10);
        if (white_thread != RT_NULL)
        {
            rt_thread_startup(white_thread);
            rt_kprintf("白光常亮模式已启动\n");
        }
        else
        {
            rt_kprintf("白光常亮模式线程创建失败！\n");
        }
    }
    else
    {
        rt_kprintf("白光常亮模式已在运行\n");
    }
}

void white_mode_stop(void)
{
    if (white_thread != RT_NULL)
    {
        rt_thread_delete(white_thread);
        white_thread = RT_NULL;
        rt_kprintf("白光常亮模式已停止\n");
    }
}

/* MSH命令：启动白光常亮模式 */
static int cmd_white(int argc, char **argv)
{
    white_mode_start();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_white, keep ws2812 led white);


static int cmd_white_stop(int argc, char **argv)
{
    white_mode_stop();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_white_stop, stop ws2812 white mode);