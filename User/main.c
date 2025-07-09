/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : ChatGPT
* Version            : V2.0.0
* Date               : 2025/05/27
* Description        : CH32V307 + RT-Thread 主程序
*********************************************************************************/
// 包含RTT文件
#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>

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
#include "Cardinal.h"
#include "PUMP.h"


// 包含温度头文件
#include "BSP/ADC/temp_adc.h"

int main(void)
{
    SystemCoreClockUpdate();

    // 初始化 Finsh Shell
    finsh_system_init();
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);

    rt_device_t console = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (console)
    {
        rt_device_open(console, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }

    // 初始化屏幕
    lcd_init();
    ws2812_init(); //led
    
    // 初始化WIFI和电机
    cardinal_module_init(); //主控线程
    pump_thread_init();
    pwm_module_init();  
    wifi_module_init(); 
    breathing_start(); //呼吸灯线程
    rainbow_init();
    rainbow_start(1);

    
    // 初始化温度显示
    adc_temperature_init();

    // 初始化屏幕模块并创建鱼缸UI显示线程
    screen_init();

    // 主循环
    while(1)
    {
        rt_thread_mdelay(1000);
        rt_kprintf("主线程\n");
        // 主循环保持空闲，工作由线程完成
    }
}