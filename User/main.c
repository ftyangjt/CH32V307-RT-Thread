/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : ChatGPT
* Version            : V2.0.0
* Date               : 2025/05/27
* Description        : CH32V307 + RT-Thread 主程序
*********************************************************************************/

#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>
#include "drivers/pin.h"

// 包含灯条模块头文件
#include "ws2812b/ws2812.h"
#include "ws2812b/rainbow.h"

// 包含屏幕模块头文件
#include "BSP/LCD/lcd.h"
#include "screen/screen.h"

// 包含WIFI模块头文件
#include "WIFI.h"
#include "PWM.h"
#include "drv_pwm.h"
#include "shell.h"

int main(void)
{
    SystemCoreClockUpdate();

    // 初始化屏幕
    lcd_init();

    // 初始化彩虹灯光模块
    ws2812_init();
    rainbow_init();
    rainbow_start(3);  // 启动彩虹效果，速度级别3

    // 初始化WIFI和电机
    pwm_module_init();  
    wifi_module_init(); 

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

    // 初始化屏幕模块并创建鱼缸UI显示线程
    screen_init();

    // 主循环
    while(1)
    {
        rt_thread_mdelay(1000);
        // 主循环保持空闲，工作由线程完成
    }
}