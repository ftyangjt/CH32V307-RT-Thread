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

// 包含模块头文件
#include "ws2812b/ws2812.h"
#include "ws2812b/rainbow.h"

// 包含屏幕头文件
#include "lcd/lcd_rt.h"

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

// LCD显示线程
static void lcd_show_entry(void *parameter)
{
     lcd_hw_init();
    // rt_kprintf("lcd_show_entry run!\n");
    // lcd_hw_clear(0xFFFF); // 白色清屏
    // lcd_hw_show_string(10, 10, "Hello RT-Thread!", 0xF800); // 红色
    // lcd_hw_show_string(10, 40, "LCD Test", 0x001F);         // 蓝色
    // lcd_hw_show_string(10, 70, "正点原子", 0x07E0);          // 绿色

    // 线程任务完成后挂起自己
    rt_thread_suspend(rt_thread_self());
    rt_schedule();
}

int main(void)
{
    rt_kprintf("\r\n");
    rt_kprintf("******************************************************\r\n");
    rt_kprintf("* MCU: CH32V307                                      *\r\n");
    rt_kprintf("* System: V2.0.0                                     *\r\n");
    rt_kprintf("* Date: 2025/05/27                                   *\r\n");
    rt_kprintf("******************************************************\r\n");
    
    SystemCoreClockUpdate();
    rt_kprintf("SysClk: %dHz\r\n", SystemCoreClock);
    rt_kprintf("ChipID: %08x\r\n", DBGMCU_GetCHIPID());
    


    // 创建LCD显示线程
    rt_thread_t tid = rt_thread_create("lcd_show",
                                       lcd_show_entry,
                                       RT_NULL,
                                       1024,
                                       10,
                                       10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        rt_kprintf("RT_NULL!!");
    
    // 初始化各模块
    ws2812_init();
    rainbow_init();
    // 启动彩虹效果
    rainbow_start(3);  // 速度级别3

    // 主循环
    while(1)
    {
        rt_thread_mdelay(1000);
        // 主循环保持空闲，工作由线程完成
    }
}