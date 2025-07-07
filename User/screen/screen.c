/********************************** (C) COPYRIGHT *******************************
* File Name          : screen.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/06/27
* Description        : 屏幕显示模块实现文件
*********************************************************************************/

#include "screen.h"

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
 * @brief       在右下角绘制鱼的轮廓
 * @param       无
 * @retval      无
 */
void draw_fish_outline(void)
{
    uint32_t fish_color = LIGHT_BG; // 使用浅色作为鱼的颜色
    
    // 鱼身体轮廓（椭圆形）
    // 上半部分轮廓
    for(int x = 390; x <= 450; x++)
    {
        int y_offset = (int)(15 * sqrt(1 - pow((x - 420.0) / 30.0, 2)));
        lcd_draw_point(x, 250 - y_offset, fish_color); // 上轮廓
        lcd_draw_point(x, 250 + y_offset, fish_color); // 下轮廓
    }
    
    // 鱼头部（圆弧）
    for(int angle = -60; angle <= 60; angle++)
    {
        float rad = angle * 3.14159 / 180.0;
        int x = 450 + (int)(15 * cos(rad));
        int y = 250 + (int)(15 * sin(rad));
        if(x >= 380 && x < 480 && y >= 220 && y < 320)
            lcd_draw_point(x, y, fish_color);
    }
    
    // 鱼尾巴（三角形）
    // 上尾翼
    for(int i = 0; i <= 20; i++)
    {
        int x = 390 - i;
        int y = 235 - i;
        if(x >= 380 && y >= 220)
            lcd_draw_point(x, y, fish_color);
    }
    
    // 下尾翼
    for(int i = 0; i <= 20; i++)
    {
        int x = 390 - i;
        int y = 265 + i;
        if(x >= 380 && y < 320)
            lcd_draw_point(x, y, fish_color);
    }
    
    // 尾翼连接线
    for(int x = 370; x <= 390; x++)
    {
        if(x >= 380)
        {
            lcd_draw_point(x, 235 - (390-x), fish_color); // 上尾翼边
            lcd_draw_point(x, 265 + (390-x), fish_color); // 下尾翼边
        }
    }
    
    // 鱼眼睛
    for(int dx = -2; dx <= 2; dx++)
    {
        for(int dy = -2; dy <= 2; dy++)
        {
            if(dx*dx + dy*dy <= 4)
                lcd_draw_point(440 + dx, 245 + dy, BG_TOP); // 用深色画眼睛
        }
    }
    
    // 鱼鳃（弧线）
    for(int i = 0; i <= 10; i++)
    {
        lcd_draw_point(435, 240 + i, fish_color);
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

    // 初始时间（可根据实际需求修改）
    int hour = 10, min = 25, sec = 0;

    while (1)
    {
        // 刷新UI界面
        display_aquarium_ui();

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
