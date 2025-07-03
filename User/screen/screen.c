/********************************** (C) COPYRIGHT *******************************
* File Name          : screen.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/06/27
* Description        : 屏幕显示模块实现文件
*********************************************************************************/

#include "screen.h"

/**
 * @brief       显示鱼缸状态UI界面
 * @param       无
 * @retval      无
 */
void display_aquarium_ui(void)
{
    lcd_draw_gradient_bg();

    // 顶部时间（左上角，大号字体）
    text_show_string(25, 33, 160, 53, "10:25", 32, 1, LIGHT_BG);

    // 顶部“AQUARIUM STATUS”（右上角）
    text_show_string(240, 32, 220, 36, "AQUARIUM STATUS", 24, 1, LIGHT_BG);

    // 第一条水平分隔线
    lcd_draw_line(20, 80, 460, 80, LIGHT_BG);

    // 温度大号显示（居中）
    text_show_string(200, 120, 200, 60, "25.5°C", 32, 1, LIGHT_BG);

    // 第二条水平分隔线
    lcd_draw_line(20, 200, 460, 200, LIGHT_BG);

    // 下次喂食信息（底部居中）
    text_show_string(40, 230, 260, 36, "NEXT FEEDING", 24, 1, LIGHT_BG);
    text_show_string(300, 230, 160, 36, "3:00 PM", 32, 1, LIGHT_BG);
}

/**
 * @brief       更新时间显示
 * @param       无
 * @retval      无
 */

/**
 * @brief       鱼缸状态显示线程入口函数
 * @param       parameter : 线程参数
 * @retval      无
 */
void pic_show_thread_entry(void *parameter)
{
    // 初始化图片库
    piclib_init();
    
    // 显示鱼缸状态UI
    display_aquarium_ui();
    
    // 主循环 - 每秒更新时间显示，LED闪烁表示系统运行
    
    while (1)
    {
        LED0_TOGGLE();
        rt_thread_mdelay(1000);
        
    }
}

/**
 * @brief       初始化屏幕模块并创建UI显示线程
 * @param       无
 * @retval      线程句柄
 */
rt_thread_t screen_init(void)
{
    rt_thread_t ui_tid = rt_thread_create("aquarium_ui",
                                          pic_show_thread_entry,
                                          RT_NULL,
                                          2048,
                                          15,
                                          10);
    if (ui_tid != RT_NULL)
    {
        rt_thread_startup(ui_tid);
    }
    
    return ui_tid;
}

static uint16_t color_lerp(uint16_t c1, uint16_t c2, float t)
{
    int r = RGB565_R(c1) + (RGB565_R(c2) - RGB565_R(c1)) * t;
    int g = RGB565_G(c1) + (RGB565_G(c2) - RGB565_G(c1)) * t;
    int b = RGB565_B(c1) + (RGB565_B(c2) - RGB565_B(c1)) * t;
    return RGB565(r, g, b);
}

void lcd_draw_gradient_bg(void)
{
    for(int y=0; y<320; y++)
    {
        float t = (float)y / 319.0f;
        uint16_t color = color_lerp(BG_TOP, BG_BOTTOM, t);
        lcd_fill(0, y, 479, y, color); // 填充一行
    }
}