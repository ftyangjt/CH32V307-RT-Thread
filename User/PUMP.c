#include <rtdevice.h>
#include <board.h> 
#include <rtthread.h>
#include <drv_gpio.h> // 添加此行
#include "Cardinal.h"

//需根据实际硬件修改
#define RELAY_PIN    87

void pump_on(void)
{
    rt_pin_write(RELAY_PIN, PIN_HIGH);
}

void pump_off(void)
{
    rt_pin_write(RELAY_PIN, PIN_LOW);
}
void submersible_pump_init(void)
{
    rt_pin_mode(RELAY_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(RELAY_PIN, PIN_LOW);
}

// 泵控制线程
static void pump_thread_entry(void *parameter)
{
    extern struct rt_semaphore *g_pump_sem;
    extern struct rt_semaphore *g_pump_done_sem;
    while (1)
    {
        // 等待信号量
        rt_sem_take(g_pump_sem, RT_WAITING_FOREVER);

        pump_on();
        rt_kprintf("水泵已开启，持续 %d 秒\n", g_water);
        rt_thread_mdelay(1000 * g_water);
        pump_off();
        rt_kprintf("水泵已关闭\n");
        rt_sem_release(g_pump_done_sem);
    }
}

// 创建泵控制线程的函数
void pump_thread_init(void)
{
    submersible_pump_init();
    rt_thread_t tid = rt_thread_create("pump",
                                      pump_thread_entry,
                                      RT_NULL,
                                      1024,
                                      7,
                                      10);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
}



