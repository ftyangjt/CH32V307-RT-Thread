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

int g_light = 1;        
int g_water = 10;

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
static int g_cardinal_test_L = 1;
static int g_cardinal_test_W = 10;
static rt_bool_t g_cardinal_test_flag = RT_FALSE;

// 增加全局变量，控制时间是否可被更新
static rt_bool_t g_cardinal_time_stop = RT_FALSE;

//它，仅检测时间是否到达任务点，不流逝时间
static void cardinal_time_thread(void *parameter)
{
    int last_hour = -1, last_minute = -1;
    int last_feed_hour = -1; // 新增变量，记录上次喂食的小时
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
                // 新增：避免同一小时重复喂食
                if (last_feed_hour != g_cardinal_time.hour)
                {
                    // 启动主控线程
                    rt_sem_release(&cardinal_main_sem);
                    last_feed_hour = g_cardinal_time.hour;
                }
            }
            // 新增：如果分钟不为0，允许下一个整点重新触发
            if (g_cardinal_time.minute != 0)
            {
                last_feed_hour = -1;
            }
        }
        rt_thread_mdelay(6000); // 检查频率
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
            g_light = g_cardinal_test_L;
            g_water = g_cardinal_test_W;
            g_cardinal_test_flag = RT_FALSE;
            rt_kprintf("[Cardinal] (测试) 主控线程被手动触发, amount=%d, light=%d, water=%d\n", amount, g_light, g_water);
        }
        else
        {
            amount = g_cardinal_tasks[g_cardinal_time.hour].amount;
            g_light = g_cardinal_tasks[g_cardinal_time.hour].light_mode;
            g_water = g_cardinal_tasks[g_cardinal_time.hour].water_duration;
            if (amount <= 0) amount = 1;
            rt_kprintf("[Cardinal] 主控线程被定时触发: %02d:00, amount=%d, light=%d, water=%d\n",
                g_cardinal_time.hour, amount, g_light, g_water);
        }

        //灯光
        if (g_light){        
        rt_sem_release(&breathing_sem);
        rt_sem_take(&breathing_done_sem, RT_WAITING_FOREVER);
        }

        // 设置舵机参数
        g_servo_param.speed = 10 * amount;
        g_servo_param.duration_sec = 2;
        // 唤醒舵机线程
        rt_sem_release(&cardinal_servo_sem);
        rt_sem_take(&cardinal_servo_done_sem, RT_WAITING_FOREVER);

        // 唤醒泵线程
        if (g_water){        
        rt_sem_release(&pump_sem);
        rt_sem_take(&pump_done_sem, RT_WAITING_FOREVER);
        };

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
    int light = 1;
    int water = 10;
    if (argc == 2)
    {
        amount = atoi(argv[1]);
        if (amount <= 0)
        {
            rt_kprintf("[Cardinal] 错误：amount必须大于0，已设置为默认值1\n");
            amount = 1;
        }
    }
    else if (argc == 3)
    {
        amount = atoi(argv[1]);
        light = atoi(argv[2]);
        if (amount <= 0)
        {
            rt_kprintf("[Cardinal] 错误：amount必须大于0，已设置为默认值1\n");
            amount = 1;
        }
    }
    else if (argc == 4)
    {
        amount = atoi(argv[1]);
        light = atoi(argv[2]);
        water = atoi(argv[3]);
        if (amount <= 0)
        {
            rt_kprintf("[Cardinal] 错误：amount必须大于0，已设置为默认值1\n");
            amount = 1;
        }
    }
    else if (argc > 4)
    {
        rt_kprintf("[Cardinal] 错误：参数过多\n");
        rt_kprintf("用法: cardinal_trigger [amount] [light] [water]\n");
        return;
    }

    //设置测试参数
    g_cardinal_test_amount = amount;
    g_cardinal_test_L = light;
    g_cardinal_test_W = water;
    g_cardinal_test_flag = RT_TRUE;

    // 释放信号量触发主控线程
    rt_sem_release(&cardinal_main_sem);
    rt_kprintf("[Cardinal] 控制台手动触发主控线程, amount=%d, light=%d, water=%d\n", amount, light, water);
}

//校准

// 由WIFI调用，校准时间
void cardinal_set_time(int hour, int minute)
{
    if (g_cardinal_time_stop)
    {
        rt_kprintf("[Cardinal] 时间已锁定，忽略校准请求\n");
        return;
    }
    g_cardinal_time.hour = hour;
    g_cardinal_time.minute = minute;
    rt_kprintf("[Cardinal] 时间校准为 %02d:%02d\n", hour, minute);
}

// 由WIFI调用，更新定时任务表
// json: {"6":{"amount":5},"8":{"amount":1},"16":{"amount":1}}
// 字符串解析浮点数，不用%f
static float parse_float(const char *str) {
    int int_part = 0, frac_part = 0, frac_len = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str && isdigit(*str)) {
        int_part = int_part * 10 + (*str - '0');
        str++;
    }
    if (*str == '.') {
        str++;
        while (*str && isdigit(*str)) {
            frac_part = frac_part * 10 + (*str - '0');
            frac_len++;
            str++;
        }
    }
    float result = int_part;
    if (frac_len > 0) {
        float frac = frac_part;
        while (frac_len--) frac /= 10.0f;
        result += frac;
    }
    return sign * result;
}

// 提取键对应的整型值
static int extract_int(const char *key, const char *json) {
    char *pos = strstr(json, key);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    return atoi(pos + 1);
}

// 提取键对应的浮点值（用字符串切片后手动解析）
static float extract_float(const char *key, const char *json) {
    char *pos = strstr(json, key);
    if (!pos) return 0.0f;
    pos = strchr(pos, ':');
    if (!pos) return 0.0f;
    pos++;
    // 找到结束符号 , 或 }
    char buf[16] = {0};
    int i = 0;
    while (*pos && *pos != ',' && *pos != '}' && i < 15) {
        if (*pos != ' ' && *pos != '"') {
            buf[i++] = *pos;
        }
        pos++;
    }
    return parse_float(buf);
}

// 提取灯光字符串（"0"或"1"）
static int extract_light(const char *key, const char *json) {
    char *pos = strstr(json, key);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    pos++; // skip :
    while (*pos && (*pos == ' ' || *pos == '"')) pos++; // 跳过空格和引号
    if (*pos >= '0' && *pos <= '9') // 检查是否为数字
        return *pos - '0';
    return 0;
}
// 主函数：解析 JSON 并更新任务表
// 主函数：解析 JSON 并更新任务表
void cardinal_update_tasks(const char *json) {
    // 先清空所有原数据
    for (int i = 0; i < 24; i++) {
        g_cardinal_tasks[i].amount = 0;
        g_cardinal_tasks[i].light_mode = 0;
        g_cardinal_tasks[i].water_duration = 0;
    }

    for (int hour = 0; hour < 24; hour++) {
        char hour_key[8];
        snprintf(hour_key, sizeof(hour_key), "\"%d\"", hour);
        char *entry = strstr(json, hour_key);
        if (!entry) continue;

        // 定位当前小时对应的 JSON 子串
        char *entry_start = strchr(entry, '{');
        char *entry_end   = strchr(entry, '}');
        if (!entry_start || !entry_end || entry_end < entry_start) continue;

        char entry_json[128] = {0};
        int len = entry_end - entry_start + 1;
        if (len >= sizeof(entry_json)) len = sizeof(entry_json) - 1;
        strncpy(entry_json, entry_start, len);
        entry_json[len] = '\0';

        g_cardinal_tasks[hour].amount         = extract_float("amount", entry_json);
        g_cardinal_tasks[hour].light_mode     = extract_light("light", entry_json);
        g_cardinal_tasks[hour].water_duration = extract_int("water", entry_json);
        rt_kprintf("[Cardinal] 数据更新");
    }
}

// 控制台命令：锁定时间为10:00并停止更新时间
static void cardinal_stop_time(int argc, char **argv)
{
    g_cardinal_time_stop = RT_TRUE;
    g_cardinal_time.hour = 10;
    g_cardinal_time.minute = 0;
    rt_kprintf("[Cardinal] 时间已锁定为10:00，后续不再更新时间\n");
}
MSH_CMD_EXPORT(cardinal_stop_time, 锁定时间为10:00并停止更新时间);

MSH_CMD_EXPORT(cardinal_trigger, 手动触发主控线程: cardinal_trigger [amount] [light] [water]);

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