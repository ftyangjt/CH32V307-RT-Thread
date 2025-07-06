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
    lcd_clear(DARK_BLUE);

    // 顶部时间（左上角，大号字体）
    text_show_string(25, 33, 160, 53, "10:25:00", 32, 1, LIGHT_BG);

    // 顶部“AQUARIUM STATUS”（右上角）
    text_show_string(240, 32, 220, 36, "AQUARIUM STATUS", 24, 1, LIGHT_BG);

    // 第一条水平分隔线
    lcd_draw_line(20, 80, 460, 80, LIGHT_BG);

    // 温度大号显示（居中）
    text_show_string(20, 130, 300, 60, "Current Temperature:", 24, 1, LIGHT_BG);

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

    // 初始时间（可根据实际需求修改）
    int hour = 10, min = 25, sec = 0;

    while (1)
    {
        // 更新时间显示
        update_time_display(hour, min, sec);

        // 动态温度显示
        update_temperature_display();

        // 时间递增
        sec++;
        if (sec >= 60)
        {
            sec = 0;
            min++;
            if (min >= 60)
            {
                min = 0;
                hour++;
                if (hour >= 24)
                    hour = 0;
            }
        }

        rt_thread_mdelay(1000);
    }
}


void update_time_display(int hour, int min, int sec)
{
    char time_str[16];
    rt_snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, min, sec);
    // 覆盖原时间区域，先用背景色清除
    lcd_fill(25, 33, 25+160, 33+33, DARK_BLUE);
    text_show_string(25, 33, 160, 53, time_str, 32, 1, LIGHT_BG);
}

void update_temperature_display(void)
{
    int temp_raw = adc_get_temperature(); // 例如 2534 表示 25.34°C
    int temp_int = temp_raw / 100;
    int temp_frac = temp_raw % 100;
    char temp_str[16];
    rt_snprintf(temp_str, sizeof(temp_str), "%d.%02d°C", temp_int, temp_frac);

    // 覆盖原温度区域，先用背景色清除
    lcd_fill(300, 125, 200+200, 120+60, DARK_BLUE);
    text_show_string(300, 125, 200, 60, temp_str, 32, 1, LIGHT_BG);
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
