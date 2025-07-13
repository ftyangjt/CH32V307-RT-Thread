#include <rtdevice.h>
#include <board.h> 
#include <rtthread.h>
#include <drv_gpio.h> // ��Ӵ���
#include "Cardinal.h"

//�����ʵ��Ӳ���޸�
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

// �ÿ����߳�
static void pump_thread_entry(void *parameter)
{
    extern struct rt_semaphore *g_pump_sem;
    extern struct rt_semaphore *g_pump_done_sem;
    while (1)
    {
        // �ȴ��ź���
        rt_sem_take(g_pump_sem, RT_WAITING_FOREVER);

        pump_on();
        rt_kprintf("ˮ���ѿ��������� %d ��\n", g_water);
        rt_thread_mdelay(1000 * g_water);
        pump_off();
        rt_kprintf("ˮ���ѹر�\n");
        rt_sem_release(g_pump_done_sem);
    }
}

// �����ÿ����̵߳ĺ���
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



