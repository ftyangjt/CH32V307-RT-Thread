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
static rt_thread_t solid_color_thread = RT_NULL;  // 重命名为solid_color_thread

// 全局颜色参数，用于存储当前纯色模式的RGB值
static struct {
    uint8_t r;
    uint8_t g; 
    uint8_t b;
    uint8_t brightness;  // 亮度百分比(0-100)
} solid_color = {255, 255, 255, 100};  // 默认为白色，100%亮度


// 自定义字符串转整数函数
int str_to_int(const char* str)
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
    
    // 初始化呼吸效果参数
    rainbow_ctrl.breathing_enabled = 0;  // 默认关闭呼吸效果
    rainbow_ctrl.breathing_cycle_ms = 3000;  // 默认3秒一个周期
    rainbow_ctrl.min_brightness = 5;  // 最小亮度为5%
    
    rt_kprintf("彩虹效果已初始化\n");
}

/* 彩虹效果线程 */
static void rainbow_thread_entry(void* parameter)
{
    ws2812_hsv_t hsv;
    hsv.s = 1.0f;
    float hue = 0.0f;
    float angle = 0.0f;  // 呼吸效果的角度
    
    while(1) {
        // 计算呼吸效果的亮度调制因子(0.0-1.0)
        float breathing_factor = 1.0f;
        
        if (rainbow_ctrl.breathing_enabled) {
            // 使用正弦波产生呼吸效果
            breathing_factor = (sinf(angle) + 1.0f) / 2.0f;  // 将-1到1映射到0到1
            
            // 如果配置了最小亮度，则将呼吸范围设为最小亮度到最大亮度
            if (rainbow_ctrl.min_brightness > 0) {
                float min_factor = (float)rainbow_ctrl.min_brightness / 100.0f;
                breathing_factor = min_factor + breathing_factor * (1.0f - min_factor);
            }
            
            // 更新角度
            angle += 2 * PI / (rainbow_ctrl.breathing_cycle_ms / rainbow_ctrl.delay_ms);
            if (angle >= 2 * PI) angle -= 2 * PI;
        }
        
        // 设置亮度(0-100转换为0.0-1.0)，加入呼吸调制
        hsv.v = (float)rainbow_ctrl.brightness / 100.0f * breathing_factor;
        
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
    // 先停止纯色模式
    solid_color_stop();
    
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

/* 纯色模式线程 */
static void solid_color_thread_entry(void* parameter)
{
    uint8_t color_data[WS2812_LED_NUM * 3];

    while (1)
    {
        // 根据亮度调整RGB值
        uint8_t r = (uint8_t)((solid_color.r * solid_color.brightness) / 100);
        uint8_t g = (uint8_t)((solid_color.g * solid_color.brightness) / 100);
        uint8_t b = (uint8_t)((solid_color.b * solid_color.brightness) / 100);
        
        // 填充所有LED为设定的纯色
        for (int i = 0; i < WS2812_LED_NUM; i++)
        {
            color_data[i * 3 + 0] = r; // R
            color_data[i * 3 + 1] = g; // G
            color_data[i * 3 + 2] = b; // B
        }
        ws2812_update(color_data);
        rt_thread_mdelay(100); // 保持刷新
    }
}

/* 启动纯色模式 */
void solid_color_start(void)
{
    rainbow_stop(); // 先停止彩虹效果线程

    if (solid_color_thread == RT_NULL)
    {
        solid_color_thread = rt_thread_create("solid_color",
                                        solid_color_thread_entry,
                                        RT_NULL,
                                        512,
                                        9,
                                        10);
        if (solid_color_thread != RT_NULL)
        {
            rt_thread_startup(solid_color_thread);
            rt_kprintf("纯色模式已启动，RGB:(%d,%d,%d)，亮度:%d%%\n", 
                      solid_color.r, solid_color.g, solid_color.b, solid_color.brightness);
        }
        else
        {
            rt_kprintf("纯色模式线程创建失败！\n");
        }
    }
    else
    {
        rt_kprintf("纯色模式已在运行\n");
    }
}

/* 停止纯色模式 */
void solid_color_stop(void)
{
    if (solid_color_thread != RT_NULL)
    {
        rt_thread_delete(solid_color_thread);
        solid_color_thread = RT_NULL;
        rt_kprintf("纯色模式已停止\n");
    }
}

/* 设置纯色模式的RGB值 */
void solid_color_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    solid_color.r = r;
    solid_color.g = g;
    solid_color.b = b;
    
    rt_kprintf("纯色模式颜色已设置为RGB:(%d,%d,%d)\n", r, g, b);
    
    // 如果纯色模式正在运行，更新颜色
    if (solid_color_thread != RT_NULL) {
        // 无需重启线程，线程内部会使用最新的颜色值
    }
}

/* 设置纯色模式的亮度 */
void solid_color_set_brightness(uint8_t brightness)
{
    if(brightness > 100) brightness = 100;
    solid_color.brightness = brightness;
    
    rt_kprintf("纯色模式亮度已设置为%d%%\n", brightness);
}

/* 为了向后兼容，保留白光模式函数名 */
void white_mode_start(void)
{
    solid_color_set_rgb(255, 255, 255); // 设为白色
    solid_color_start();
}

void white_mode_stop(void)
{
    solid_color_stop();
}

/* MSH命令：启动白光模式 */
static int cmd_white(int argc, char **argv)
{
    white_mode_start();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_white, keep ws2812 led white);

/* MSH命令：停止纯色模式 */
static int cmd_white_stop(int argc, char **argv)
{
    solid_color_stop();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_white_stop, stop ws2812 white mode);

/* MSH命令：设置纯色模式 */
static int cmd_color(int argc, char **argv)
{
    if (argc < 4) {
        rt_kprintf("用法: color r g b [brightness]\n");
        rt_kprintf("示例: color 255 0 0 50  设置为红色，亮度50%%\n");
        return -RT_ERROR;
    }
    
    int r = str_to_int(argv[1]);
    int g = str_to_int(argv[2]);
    int b = str_to_int(argv[3]);
    
    // 检查RGB范围
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    
    solid_color_set_rgb(r, g, b);
    
    // 设置亮度（可选参数）
    if (argc > 4) {
        int brightness = str_to_int(argv[4]);
        solid_color_set_brightness(brightness);
    }
    
    // 启动纯色模式
    solid_color_start();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_color, set ws2812 led color: color r g b [brightness]);

/* MSH命令：设置纯色亮度 */
static int cmd_color_brightness(int argc, char **argv)
{
    int brightness = 100;  // 默认亮度100%
    
    if(argc > 1) {
        brightness = str_to_int(argv[1]);
        if(brightness < 0) brightness = 0;
        if(brightness > 100) brightness = 100;
    }
    
    solid_color_set_brightness(brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_color_brightness, set solid color brightness [0-100]);


/* 启动带呼吸效果的彩虹效果 */
void rainbow_breathing_start(uint8_t speed_level, uint32_t breathing_cycle_s)
{
    // 停止其他模式
    solid_color_stop();
    rainbow_stop();
    
    // 设置呼吸周期
    if (breathing_cycle_s < 1) breathing_cycle_s = 1;
    if (breathing_cycle_s > 60) breathing_cycle_s = 60;
    rainbow_ctrl.breathing_cycle_ms = breathing_cycle_s * 1000;
    
    // 开启呼吸效果
    rainbow_ctrl.breathing_enabled = 1;
    
    // 启动彩虹效果
    rainbow_start(speed_level);
    
    rt_kprintf("彩虹呼吸效果已启动，速度级别=%d，呼吸周期=%d秒\n", 
               speed_level, breathing_cycle_s);
}

/* 设置彩虹呼吸效果的最小亮度 */
void rainbow_set_min_brightness(uint8_t min_brightness)
{
    if (min_brightness >= rainbow_ctrl.brightness) {
        min_brightness = rainbow_ctrl.brightness / 2;  // 防止最小亮度大于等于最大亮度
    }
    
    rainbow_ctrl.min_brightness = min_brightness;
    rt_kprintf("彩虹呼吸效果最小亮度已设置为%d%%\n", min_brightness);
}

/* MSH命令：启动彩虹呼吸效果 */
static int cmd_rainbow_breathing(int argc, char **argv)
{
    int speed = 3;  // 默认速度级别
    int cycle = 3;  // 默认3秒呼吸周期
    
    if (argc > 1) {
        speed = str_to_int(argv[1]);
    }
    
    if (argc > 2) {
        cycle = str_to_int(argv[2]);
    }
    
    rainbow_breathing_start(speed, cycle);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_breathing, start rainbow breathing effect [speed 1-5] [cycle_seconds]);

/* MSH命令：设置彩虹呼吸效果最小亮度 */
static int cmd_rainbow_min_brightness(int argc, char **argv)
{
    int min_brightness = 10;  // 默认最小亮度10%
    
    if (argc > 1) {
        min_brightness = str_to_int(argv[1]);
        if (min_brightness < 0) min_brightness = 0;
        if (min_brightness > 50) min_brightness = 50;  // 最大不超过50%
    }
    
    rainbow_set_min_brightness(min_brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_min_brightness, set rainbow breathing min brightness [0-50]);