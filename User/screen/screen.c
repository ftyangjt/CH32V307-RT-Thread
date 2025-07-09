/********************************** (C) COPYRIGHT *******************************
* File Name          : screen.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/06/27
* Description        : 屏幕显示模块实现文件
*********************************************************************************/

#include "screen.h"
#include "../Cardinal.h"

// 提取RGB565分量
#define RGB565_R(c) (((c) >> 11) & 0x1F)
#define RGB565_G(c) (((c) >> 5) & 0x3F)
#define RGB565_B(c) ((c) & 0x1F)

// 合成RGB565
#define RGB565(r,g,b) (((r)<<11)|((g)<<5)|(b))

// 线性插值
static uint16_t color_lerp(uint16_t c1, uint16_t c2, float t)
{
    int r = RGB565_R(c1) + (RGB565_R(c2) - RGB565_R(c1)) * t;
    int g = RGB565_G(c1) + (RGB565_G(c2) - RGB565_G(c1)) * t;
    int b = RGB565_B(c1) + (RGB565_B(c2) - RGB565_B(c1)) * t;
    return RGB565(r, g, b);
}

#define BG_TOP    0x21CA   // 深蓝
#define BG_BOTTOM 0x4F7D   // 浅蓝

void lcd_draw_gradient_bg(void)
{
    for(int y=0; y<320; y++)
    {
        float t = (float)y / 319.0f;
        uint16_t color = color_lerp(BG_TOP, BG_BOTTOM, t);
        lcd_fill(0, y, 479, y, color); // 填充一行
    }
}

/**
 * @brief       绘制红黑渐变背景（喂食状态）
 * @param       无
 * @retval      无
 */
void lcd_draw_feeding_gradient_bg(void)
{
    for(int y=0; y<320; y++)
    {
        float t = (float)y / 319.0f;
        uint16_t color = color_lerp(FEEDING_TOP, FEEDING_BOTTOM, t);
        lcd_fill(0, y, 479, y, color); // 填充一行
    }
}

/**
 * @brief       获取指定Y坐标的渐变背景色
 * @param       y: Y坐标
 * @retval      该位置的背景色
 */
static uint16_t get_gradient_color_at_y(int y)
{
    if (y < 0) y = 0;
    if (y >= 320) y = 319;
    
    float t = (float)y / 319.0f;
    return color_lerp(BG_TOP, BG_BOTTOM, t);
}

/**
 * @brief       填充指定区域的渐变背景
 * @param       x1,y1,x2,y2: 矩形区域坐标
 * @retval      无
 */
static void fill_gradient_rect(int x1, int y1, int x2, int y2)
{
    for(int y = y1; y <= y2; y++)
    {
        uint16_t color = get_gradient_color_at_y(y);
        lcd_fill(x1, y, x2, y, color); // 填充一行
    }
}

/**
 * @brief       显示鱼缸状态UI界面
 * @param       无
 * @retval      无
 */
void display_aquarium_ui(void)
{   
    lcd_draw_gradient_bg();

    text_show_string(240, 32, 220, 36, "AQUARIUM STATUS", 24, 1, LIGHT_BG);

    // 第一条水平分隔线
    lcd_draw_line(20, 80, 460, 80, LIGHT_BG);

    // 温度大号显示（居中）
    text_show_string(20, 130, 300, 60, "Current Temperature:", 24, 1, LIGHT_BG);

    // 第二条水平分隔线
    lcd_draw_line(20, 200, 460, 200, LIGHT_BG);

    // 下次喂食信息（底部居中）
    text_show_string(40, 230, 260, 36, "NEXT FEEDING", 24, 1, LIGHT_BG);
    text_show_string(300, 230, 160, 36, "10:26 PM", 32, 1, LIGHT_BG);

}


/**
 * @brief       显示鱼缸状态UI界面（中文版）
 * @param       无
 * @retval      无
 */
void display_aquarium_ui_chi(void)
{   
    lcd_draw_gradient_bg();

    // 顶部“AQUARIUM STATUS”（右上角）
    text_show_string(240, 32, 220, 36, "智能鱼缸监控", 24, 1, LIGHT_BG);

    // 第一条水平分隔线
    lcd_draw_line(20, 80, 460, 80, LIGHT_BG);

    // 温度大号显示（居中）
    text_show_string(20, 130, 300, 60, "当前室内温度:", 24, 1, LIGHT_BG);

    // 第二条水平分隔线
    lcd_draw_line(20, 200, 460, 200, LIGHT_BG);

    // 下次喂食信息（底部居中）
    text_show_string(40, 230, 260, 36, "下一次喂食时间", 24, 1, LIGHT_BG);
    text_show_string(300, 230, 160, 36, "10:26 AM", 32, 1, LIGHT_BG);

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

    // 初始时间（可根据实际需求修改）
    int hour = 10, min = 25, sec = 0;
    
    // 记录上一次的喂食状态
    int last_feeding_state = 0;

    while (1)
    {   
        // 检查喂食状态是否发生变化
        if (onFeeding != last_feeding_state)
        {
            last_feeding_state = onFeeding;
            
            if (onFeeding)
            {
                // 切换到红黑渐变背景，不显示其他内容
                lcd_draw_feeding_gradient_bg();
            }
            else
            {
                // 恢复正常界面
                display_aquarium_ui();
            }
        }
        
        if (onFeeding)
        {
            // 喂食状态下只保持红黑渐变背景，不做其他更新
        }
        else
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
        }
        
        rt_thread_mdelay(1000);
    }
}



void update_time_display(int hour, int min, int sec)
{
    char time_str[16];
    rt_snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, min, sec);

    // 先用渐变背景色清除时间显示区域
    fill_gradient_rect(25, 33, 25+160, 33+33);

    text_show_string(25, 33, 160, 53, time_str, 32, 1, LIGHT_BG);
}

void update_temperature_display(void)
{
    int temp_raw = adc_get_temperature(); // 例如 2534 表示 25.34°C
    int temp_int = temp_raw / 100;
    int temp_frac = temp_raw % 100;
    char temp_str[16];
    rt_snprintf(temp_str, sizeof(temp_str), "%d.%02d°C", temp_int, temp_frac);

    // 先用渐变背景色清除温度显示区域
    fill_gradient_rect(300, 125, 300+200, 125+60);
    
    text_show_string(300, 125, 200, 60, temp_str, 32, 1, LIGHT_BG);
}

/**
 * @brief       初始化屏幕模块并创建UI显示线程
 * @param       无
 * @retval      线程句柄
 */
rt_thread_t screen_init(void)
{
    my_mem_init(SRAMIN);                                /* 初始化内部SRAM内存池 */
    exfuns_init();                                      /* 为fatfs相关变量申请内存 */
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
                                          2048,
                                          15,
                                          10);
    if (ui_tid != RT_NULL)
    {
        rt_thread_startup(ui_tid);
    }
    
    return ui_tid;
}
