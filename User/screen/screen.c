/********************************** (C) COPYRIGHT *******************************
* File Name          : screen.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/06/27
* Description        : 屏幕显示模块实现文件
*********************************************************************************/

#include "screen.h"
#include "../Cardinal.h"

/**
 * @brief       显示喂食状态界面
 * @param       无
 * @retval      无
 */
void display_feeding_ui(void)
{
    // 屏幕中间显示"FEEDING"
    // 屏幕尺寸 480x320，居中显示
    text_show_string(170, 140, 140, 48, "FEEDING", 48, 1, LIGHT_BG);
}

/**
 * @brief       显示鱼缸状态UI界面
 * @param       无
 * @retval      无
 */
void display_aquarium_ui(void)
{   
    // 清空屏幕
    lcd_clear(BLACK);
    
    // 加载背景显示背景图片
    char bg_path[] = "0:/PICTURE/background.bmp";
    uint8_t res = piclib_ai_load_picfile(bg_path, 0, 0, lcddev.width, lcddev.height, 1);
    
    if (res != 0) 
    {
        // 显示错误显示背景选项
        text_show_string(10, 10, 200, 16, "背景图片加载失败", 16, 1, RED);
    }

    text_show_string(10, 10, 150, 48, "10:32", 48, 1, BLACK);

    text_show_string(240, 10, 150, 24, "当前温度: ", 24, 1, BLACK);

    text_show_string(350, 10, 150, 48, "66.66", 48, 1, BLACK);

    text_show_string(190, 70, 100, 32, "下次喂食时间", 24, 1, BLACK);

    text_show_string(200, 100, 100, 48, "3:00", 32, 1, BLACK);

}

/**
 * @brief       鱼缸状态显示线程入口函数
 * @param       parameter : 线程参数
 * @retval      无
 */
void pic_show_thread_entry(void *parameter)
{
    // 初始化图片库
    piclib_init();

    display_aquarium_ui();

    // 初始时间（可根据实际情况修改）
    int hour = 10, min = 25, sec = 0;
    
    // 记录上一次的喂食状态
    int last_feeding_state = 0;

    while (1)
    {   
        // 检测喂食状态是否发生变化
        if (onFeeding != last_feeding_state)
        {
            last_feeding_state = onFeeding;
            
            if (onFeeding)
            {
                // 切换到黑色渐变背景并显示喂食界面
                display_feeding_ui();
            }
            else
            {
                // 恢复鱼缸界面
                display_aquarium_ui();
            }
        }
        
        if (onFeeding)
        {
            // 喂食状态下只保持黑色渐变背景，不更新其他信息
        }
        else
        {
            // 动态时间显示
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
        }
        
        rt_thread_mdelay(1000);
    }
}

// void pic_show_thread_entry(void *parameter)
// {
//     // 初始化图片库
//     piclib_init();

//     display_aquarium_ui();
// }

void update_time_display(int hour, int min, int sec)
{
    char time_str[16];
    rt_snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, min, sec);

    // 设置背景色清除时间显示区域
    lcd_fill(10, 10, 10+150, 10+48, BG_TOP);
    text_show_string(10, 10, 150, 48, time_str, 48, 1, BLACK);

}

void update_temperature_display(void)
{
    int temp_raw = adc_get_temperature(); // 例如 2534 表示 25.34°C
    int temp_int = temp_raw / 100;
    int temp_frac = temp_raw % 100;
    char temp_str[16];
    rt_snprintf(temp_str, sizeof(temp_str), "%d.%02d°C", temp_int, temp_frac);

    // 设置背景色清除温度显示区域
    lcd_fill(350, 10, 350+150, 10+48, BG_BOTTOM);
    
    text_show_string(350, 10, 150, 48, temp_str, 48, 1, BLACK);
}

/**
 * @brief       初始化屏幕模块并创建UI显示线程
 * @param       无
 * @retval      线程句柄
 */
rt_thread_t screen_init(void)
{
    my_mem_init(SRAMIN);                                /* 初始化内部SRAM内存池 */
    exfuns_init();                                      /* 为fatfs相关申请内存 */
    f_mount(fs[0], "0:", 1);                            /* 挂载SD卡 */
    f_mount(fs[1], "1:", 1);                            /* 挂载FLASH */

    while (fonts_init())                                /* 检查字库 */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 50, 240, 66, WHITE);               /* 清除显示 */
        rt_thread_mdelay(200);
    }
    
    rt_thread_t ui_tid = rt_thread_create("aquarium_ui",
                                          pic_show_thread_entry,
                                          RT_NULL,
                                          1024,
                                          15,
                                          10);
    if (ui_tid != RT_NULL)
    {
        rt_thread_startup(ui_tid);
    }
    
    return ui_tid;
}
