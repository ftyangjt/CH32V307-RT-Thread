#include "Cardinal.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "PWM.h"
#include <finsh.h>

//ȫ��ʱ�����
cardinal_time_t g_cardinal_time = {0, 0};

//24Сʱ�����
cardinal_task_t g_cardinal_tasks[24] = {0};

//�����߳������ź���
static struct rt_semaphore cardinal_main_sem;
//�������ⲿ����main.c��ʹ��
struct rt_semaphore *g_cardinal_main_sem = &cardinal_main_sem;

//�����ź���
struct rt_semaphore cardinal_servo_sem;
struct rt_semaphore *g_cardinal_servo_sem = &cardinal_servo_sem; //ָ�룬�������߳�extern
struct rt_semaphore cardinal_servo_done_sem;
struct rt_semaphore *g_cardinal_servo_done_sem = &cardinal_servo_done_sem;

//�ֶ����� ���ȫ�ֱ���
static int g_cardinal_test_amount = 1;
static rt_bool_t g_cardinal_test_flag = RT_FALSE;

//���������ʱ���Ƿ񵽴�����㣬������ʱ��
static void cardinal_time_thread(void *parameter)
{
    int last_hour = -1, last_minute = -1;
    while (1)
    {
        //ʱ���Ƿ�仯
        if (g_cardinal_time.hour != last_hour || g_cardinal_time.minute != last_minute)
        {
            last_hour = g_cardinal_time.hour;
            last_minute = g_cardinal_time.minute;
            // ���㴥��
            if (g_cardinal_time.minute == 0 && g_cardinal_tasks[g_cardinal_time.hour].amount > 0)
            {
                // ���������߳�
                rt_sem_release(&cardinal_main_sem);
            }
        }
        rt_thread_mdelay(500); // ���Ƶ��
    }
}

//�����̺߳���
static void cardinal_main_thread(void *parameter)
{
    while (1)
    {
        rt_sem_take(&cardinal_main_sem, RT_WAITING_FOREVER);
        int amount;
        if (g_cardinal_test_flag)
        {
            amount = g_cardinal_test_amount; //ιʳ����
            g_cardinal_test_flag = RT_FALSE;
            rt_kprintf("[Cardinal] (����) �����̱߳��ֶ�����, amount=%d\n", amount);
        }
        else
        {
            amount = g_cardinal_tasks[g_cardinal_time.hour].amount;
            if (amount <= 0) amount = 1;
            rt_kprintf("[Cardinal] �����̱߳���ʱ����: %02d:00, amount=%d\n",
                g_cardinal_time.hour, amount);
        }
        
        // ���ö������
        g_servo_param.speed = 10 * amount;
        g_servo_param.duration_sec = 2;
        // ���Ѷ���߳�
        rt_sem_release(&cardinal_servo_sem);
        rt_sem_take(&cardinal_servo_done_sem, RT_WAITING_FOREVER);
    }
}

//Cardinalģ���ܳ�ʼ��
void cardinal_module_init(void)
{
    rt_sem_init(&cardinal_main_sem, "cardsem", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&cardinal_servo_sem, "servosem", 0, RT_IPC_FLAG_FIFO);
    // ��������ʼ���������ź���
    rt_sem_init(&cardinal_servo_done_sem, "servodone", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t tid1 = rt_thread_create("card_time", cardinal_time_thread, RT_NULL, 512, 10, 10);
    if (tid1) rt_thread_startup(tid1);

    rt_thread_t tid2 = rt_thread_create("card_main", cardinal_main_thread, RT_NULL, 1024, 9, 10);
    if (tid2) rt_thread_startup(tid2);
}


// ����̨����ֶ����������߳�
static void cardinal_trigger(int argc, char **argv)
{
    int amount = 1;
    if (argc == 2)
    {
        amount = atoi(argv[1]);
        if (amount <= 0)
        {
            rt_kprintf("[Cardinal] ����amount�������0��������ΪĬ��ֵ1\n");
            amount = 1;
        }
    }
    else if (argc > 2)
    {
        rt_kprintf("[Cardinal] ���󣺲�������\n");
        rt_kprintf("�÷�: cardinal_trigger [amount]\n");
        return;
    }
    
    //���ò��Բ���
    g_cardinal_test_amount = amount;
    g_cardinal_test_flag = RT_TRUE;
    rt_base_t level = rt_hw_interrupt_disable();
    rt_hw_interrupt_enable(level);
    
    // �ͷ��ź������������߳�
    rt_sem_release(&cardinal_main_sem);
    rt_kprintf("[Cardinal] ����̨�ֶ����������߳�, amount=%d\n", amount);
}
MSH_CMD_EXPORT(cardinal_trigger, �ֶ����������߳�: cardinal_trigger [amount]);












//У׼

// ��WIFI���ã�У׼ʱ��
void cardinal_set_time(int hour, int minute)
{
    g_cardinal_time.hour = hour;
    g_cardinal_time.minute = minute;
    rt_kprintf("[Cardinal] ʱ��У׼Ϊ %02d:%02d\n", hour, minute);
}

// ��WIFI���ã����¶�ʱ�����
// json: {"6":{"amount":5},"8":{"amount":1},"16":{"amount":1}}
void cardinal_update_tasks(const char *json)
{
    for (int h = 0; h < 24; h++)
        g_cardinal_tasks[h].amount = 0; // ���

    const char *p = json;
    while ((p = strchr(p, '\"')))
    {
        int hour = atoi(p + 1);
        char *q = strstr(p, "\"amount\":");
        if (q)
        {
            int amount = atoi(q + 9);
            if (hour >= 0 && hour < 24)
                g_cardinal_tasks[hour].amount = amount;
        }
        p++;
    }
    rt_kprintf("[Cardinal] ��ʱ������Ѹ���\n");
}
