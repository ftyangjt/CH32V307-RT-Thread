#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>

#include <drv_gpio.h> // 确保包含了 RT-Thread 的 GPIO 驱动支持
#include <string.h>
#include <stdlib.h>
#include "WIFI.h"
#include "PWM.h"
#include "drv_pwm.h"
#include "shell.h"
#include "Cardinal.h"

#define LED0_PIN 10  // 使用 PC0 引脚

int main(void)
{



    rt_kprintf("\r\n MCU: CH32V307\r\n");
    SystemCoreClockUpdate();
    rt_kprintf(" SysClk: %dHz\r\n", SystemCoreClock);
    rt_kprintf(" ChipID: %08x\r\n", DBGMCU_GetCHIPID());
    rt_kprintf(" www.wch.cn\r\n");      
       
    pwm_module_init();  
    wifi_module_init(); 
    cardinal_module_init(); //主控线程

    // 单元测试 
    // SystemCoreClockUpdate();      // 更新系统核心频率（72MHz）
    // printf("SystemCoreClock: %d Hz\n", SystemCoreClock);
    // pwm_gpio_init_10();              // 初始化 PC6 为复用推挽输出
    // pwm_tim10_init(20000, 1300);   // 输出 20ms 周期，1300us 脉宽（略微反转）

    // 初始化 Finsh Shell
    finsh_system_init();
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);

    rt_device_t console = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (console)
    {
        rt_device_open(console, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }

    #ifdef RT_USING_COMPONENTS_INIT
        rt_components_init();
    #endif
    while (1)
    {
        rt_kprintf("主循环运行中...\n");
        
        rt_thread_mdelay(3000);
    }
}
