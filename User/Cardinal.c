#include "Cardinal.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "PWM.h"
#include <finsh.h>
#include "ws2812b/rainbow.h"

//全局时间变量
cardinal_time_t g_cardinal_time = {0, 0};

//24小时任务表
cardinal_task_t g_cardinal_tasks[24] = {0};

//主控线程启动信号量
static struct rt_semaphore cardinal_main_sem;
//导出给外部（如main.c）使用
struct rt_semaphore *g_cardinal_main_sem = &cardinal_main_sem;

//其它信号量
struct rt_semaphore cardinal_servo_sem;
struct rt_semaphore *g_cardinal_servo_sem = &cardinal_servo_sem; //指针，被其它线程extern
struct rt_semaphore cardinal_servo_done_sem;
struct rt_semaphore *g_cardinal_servo_done_sem = &cardinal_servo_done_sem;

static struct rt_semaphore breathing_sem;
static struct rt_semaphore breathing_done_sem;
struct rt_semaphore *g_breathing_sem = &breathing_sem;
struct rt_semaphore *g_breathing_done_sem = &breathing_done_sem;

static struct rt_semaphore pump_sem;
static struct rt_semaphore pump_done_sem;
struct rt_semaphore *g_pump_sem = &pump_sem;
struct rt_semaphore *g_pump_done_sem = &pump_done_sem;

//手动触发 相关全局变量
static int g_cardinal_test_amount = 1;
static rt_bool_t g_cardinal_test_flag = RT_FALSE;

//它，仅检测时间是否到达任务点，不流逝时间
static void cardinal_time_thread(void *parameter)
{
    int last_hour = -1, last_minute = -1;
    while (1)
    {
        //时间是否变化
        if (g_cardinal_time.hour != last_hour || g_cardinal_time.minute != last_minute)
        {
            last_hour = g_cardinal_time.hour;
            last_minute = g_cardinal_time.minute;
            // 整点触发
            if (g_cardinal_time.minute == 0 && g_cardinal_tasks[g_cardinal_time.hour].amount > 0)
            {
                // 启动主控线程
                rt_sem_release(&cardinal_main_sem);
            }
        }
        rt_thread_mdelay(500); // 检查频率
    }
}

//主控线程函数
static void cardinal_main_thread(void *parameter)
{
    while (1)
    {
        rt_sem_take(&cardinal_main_sem, RT_WAITING_FOREVER);
        int amount;
        if (g_cardinal_test_flag)
        {
            amount = g_cardinal_test_amount; //喂食数量
            g_cardinal_test_flag = RT_FALSE;
            rt_kprintf("[Cardinal] (测试) 主控线程被手动触发, amount=%d\n", amount);
        }
        else
        {
            amount = g_cardinal_tasks[g_cardinal_time.hour].amount;
            if (amount <= 0) amount = 1;
            rt_kprintf("[Cardinal] 主控线程被定时触发: %02d:00, amount=%d\n",
                g_cardinal_time.hour, amount);
        }
        rt_sem_release(&breathing_sem);
        //rt_kprintf("[Cardinal] 灯光启动"),
        rt_sem_take(&breathing_done_sem, RT_WAITING_FOREVER);
        // 设置舵机参数
        g_servo_param.speed = 10 * amount;
        g_servo_param.duration_sec = 2;
        // 唤醒舵机线程
        rt_sem_release(&cardinal_servo_sem);
        rt_sem_take(&cardinal_servo_done_sem, RT_WAITING_FOREVER);
        // 唤醒泵线程
        rt_sem_release(&pump_sem);
        rt_sem_take(&pump_done_sem, RT_WAITING_FOREVER);
    }
}

//Cardinal模块总初始化
void cardinal_module_init(void)
{
    rt_sem_init(&cardinal_main_sem, "cardsem", 0, RT_IPC_FLAG_FIFO);
    //duoji
    rt_sem_init(&cardinal_servo_sem, "servosem", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&cardinal_servo_done_sem, "servodone", 0, RT_IPC_FLAG_FIFO);
    //初始化灯光信号量
    rt_sem_init(&breathing_sem, "breathe", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&breathing_done_sem, "breathedone", 0, RT_IPC_FLAG_FIFO);
    //初始化泵信号量
    rt_sem_init(&pump_sem, "pumpsem", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&pump_done_sem, "pumpdone", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t tid1 = rt_thread_create("card_time", cardinal_time_thread, RT_NULL, 512, 10, 10);
    if (tid1) rt_thread_startup(tid1);

    rt_thread_t tid2 = rt_thread_create("card_main", cardinal_main_thread, RT_NULL, 1024, 9, 10);
    if (tid2) rt_thread_startup(tid2);
}


// 控制台命令：手动触发主控线程
static void cardinal_trigger(int argc, char **argv)
{
    int amount = 1;
    if (argc == 2)
    {
        amount = atoi(argv[1]);
        if (amount <= 0)
        {
            rt_kprintf("[Cardinal] 错误：amount必须大于0，已设置为默认值1\n");
            amount = 1;
        }
    }
    else if (argc > 2)
    {
        rt_kprintf("[Cardinal] 错误：参数过多\n");
        rt_kprintf("用法: cardinal_trigger [amount]\n");
        return;
    }
    
    //设置测试参数
    g_cardinal_test_amount = amount;
    g_cardinal_test_flag = RT_TRUE;
    
    // 释放信号量触发主控线程
    rt_sem_release(&cardinal_main_sem);
    rt_kprintf("[Cardinal] 控制台手动触发主控线程, amount=%d\n", amount);
}
MSH_CMD_EXPORT(cardinal_trigger, 手动触发主控线程: cardinal_trigger [amount]);

// 控制台命令：打印定时任务表
static void cardinal_print_tasks(int argc, char **argv)
{
    rt_kprintf("Hour\tAmount(x100)\tLight\tWater\n");
    for (int i = 0; i < 24; i++)
    {
        if (g_cardinal_tasks[i].amount > 0 ||
            g_cardinal_tasks[i].light_mode > 0 ||
            g_cardinal_tasks[i].water_duration > 0)
        {
            // 将amount乘以100转为整数显示，避免%f
            rt_kprintf("%i\t%d\t\t%i\t%i\n",
                i,
                (int)(g_cardinal_tasks[i].amount * 100),
                g_cardinal_tasks[i].light_mode,
                g_cardinal_tasks[i].water_duration);
        }
    }
}
MSH_CMD_EXPORT(cardinal_print_tasks, 打印定时任务表);

//校准

// 由WIFI调用，校准时间
void cardinal_set_time(int hour, int minute)
{
    g_cardinal_time.hour = hour;
    g_cardinal_time.minute = minute;
    rt_kprintf("[Cardinal] 时间校准为 %02d:%02d\n", hour, minute);
}

// 由WIFI调用，更新定时任务表
// json: {"6":{"amount":5},"8":{"amount":1},"16":{"amount":1}}
void cardinal_update_tasks(const char *json)
{
    for (int h = 0; h < 24; h++)
    {
        g_cardinal_tasks[h].amount = 0;
        g_cardinal_tasks[h].light_mode = 0;
        g_cardinal_tasks[h].water_duration = 0;
    }

    int hour;
    const char *p = json;
    while ((p = strchr(p, '\"')))
    {
        hour = atoi(p + 1);
        char *block_start = strchr(p, '{');
        char *block_end = strchr(block_start, '}');
        if (block_start && block_end && block_end > block_start)
        {
            float amount = 0;
            int light = 0;
            int water = 0;
            // amount
            char *q = strstr(block_start, "\"amount\":");
            if (q && q < block_end)
            {
                char buf[16] = {0};
                q += strlen("\"amount\":");
                sscanf(q, "%15[^,}]", buf);
                amount = (float)atof(buf);
            }
            // light
            q = strstr(block_start, "\"light\":");
            if (q && q < block_end)
            {
                char buf[8] = {0};
                q += strlen("\"light\":");
                while (*q == ' ' || *q == '\"') q++;
                sscanf(q, "%7[^\",]", buf); // 修正格式串
                light = atoi(buf);
            }
            // water
            q = strstr(block_start, "\"water\":");
            if (q && q < block_end)
            {
                char buf[16] = {0};
                q += strlen("\"water\":");
                sscanf(q, "%15[^,}]", buf);
                water = atoi(buf);
            }
            // minute 字段暂不处理
            if (hour >= 0 && hour < 24)
            {
                g_cardinal_tasks[hour].amount = amount;
                g_cardinal_tasks[hour].light_mode = light;
                g_cardinal_tasks[hour].water_duration = water;
            }
            p = block_end + 1;
        }
        else
        {
            break;
        }
    }
    rt_kprintf("[Cardinal] 定时任务表已更新\n");
}
