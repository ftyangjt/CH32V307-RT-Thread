#include "Cardinal.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "PWM.h"
#include <finsh.h>
#include "ws2812b/rainbow.h"

//ȫ��ʱ�����
cardinal_time_t g_cardinal_time = {0, 0};

//24Сʱ�����
cardinal_task_t g_cardinal_tasks[24] = {0};

int g_light = 1;        
int g_water = 10;

//�����߳������ź���
static struct rt_semaphore cardinal_main_sem;
//�������ⲿ����main.c��ʹ��
struct rt_semaphore *g_cardinal_main_sem = &cardinal_main_sem;

//�����ź���
struct rt_semaphore cardinal_servo_sem;
struct rt_semaphore *g_cardinal_servo_sem = &cardinal_servo_sem; //ָ�룬�������߳�extern
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

//�ֶ����� ���ȫ�ֱ���
static int g_cardinal_test_amount = 1;
static int g_cardinal_test_L = 1;
static int g_cardinal_test_W = 10;
static rt_bool_t g_cardinal_test_flag = RT_FALSE;

// ����ȫ�ֱ���������ʱ���Ƿ�ɱ�����
static rt_bool_t g_cardinal_time_stop = RT_FALSE;

//���������ʱ���Ƿ񵽴�����㣬������ʱ��
static void cardinal_time_thread(void *parameter)
{
    int last_hour = -1, last_minute = -1;
    int last_feed_hour = -1; // ������������¼�ϴ�ιʳ��Сʱ
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
                // ����������ͬһСʱ�ظ�ιʳ
                if (last_feed_hour != g_cardinal_time.hour)
                {
                    // ���������߳�
                    rt_sem_release(&cardinal_main_sem);
                    last_feed_hour = g_cardinal_time.hour;
                }
            }
            // ������������Ӳ�Ϊ0��������һ���������´���
            if (g_cardinal_time.minute != 0)
            {
                last_feed_hour = -1;
            }
        }
        rt_thread_mdelay(6000); // ���Ƶ��
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
            g_light = g_cardinal_test_L;
            g_water = g_cardinal_test_W;
            g_cardinal_test_flag = RT_FALSE;
            rt_kprintf("[Cardinal] (����) �����̱߳��ֶ�����, amount=%d, light=%d, water=%d\n", amount, g_light, g_water);
        }
        else
        {
            amount = g_cardinal_tasks[g_cardinal_time.hour].amount;
            g_light = g_cardinal_tasks[g_cardinal_time.hour].light_mode;
            g_water = g_cardinal_tasks[g_cardinal_time.hour].water_duration;
            if (amount <= 0) amount = 1;
            rt_kprintf("[Cardinal] �����̱߳���ʱ����: %02d:00, amount=%d, light=%d, water=%d\n",
                g_cardinal_time.hour, amount, g_light, g_water);
        }

        //�ƹ�
        if (g_light){        
        rt_sem_release(&breathing_sem);
        rt_sem_take(&breathing_done_sem, RT_WAITING_FOREVER);
        }

        // ���ö������
        g_servo_param.speed = 10 * amount;
        g_servo_param.duration_sec = 2;
        // ���Ѷ���߳�
        rt_sem_release(&cardinal_servo_sem);
        rt_sem_take(&cardinal_servo_done_sem, RT_WAITING_FOREVER);

        // ���ѱ��߳�
        if (g_water){        
        rt_sem_release(&pump_sem);
        rt_sem_take(&pump_done_sem, RT_WAITING_FOREVER);
        };

    }
}

//Cardinalģ���ܳ�ʼ��
void cardinal_module_init(void)
{
    rt_sem_init(&cardinal_main_sem, "cardsem", 0, RT_IPC_FLAG_FIFO);
    //duoji
    rt_sem_init(&cardinal_servo_sem, "servosem", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&cardinal_servo_done_sem, "servodone", 0, RT_IPC_FLAG_FIFO);
    //��ʼ���ƹ��ź���
    rt_sem_init(&breathing_sem, "breathe", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&breathing_done_sem, "breathedone", 0, RT_IPC_FLAG_FIFO);
    //��ʼ�����ź���
    rt_sem_init(&pump_sem, "pumpsem", 0, RT_IPC_FLAG_FIFO);
    rt_sem_init(&pump_done_sem, "pumpdone", 0, RT_IPC_FLAG_FIFO);

    rt_thread_t tid1 = rt_thread_create("card_time", cardinal_time_thread, RT_NULL, 512, 10, 10);
    if (tid1) rt_thread_startup(tid1);

    rt_thread_t tid2 = rt_thread_create("card_main", cardinal_main_thread, RT_NULL, 1024, 9, 10);
    if (tid2) rt_thread_startup(tid2);
}


// ����̨����ֶ����������߳�
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
            rt_kprintf("[Cardinal] ����amount�������0��������ΪĬ��ֵ1\n");
            amount = 1;
        }
    }
    else if (argc == 3)
    {
        amount = atoi(argv[1]);
        light = atoi(argv[2]);
        if (amount <= 0)
        {
            rt_kprintf("[Cardinal] ����amount�������0��������ΪĬ��ֵ1\n");
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
            rt_kprintf("[Cardinal] ����amount�������0��������ΪĬ��ֵ1\n");
            amount = 1;
        }
    }
    else if (argc > 4)
    {
        rt_kprintf("[Cardinal] ���󣺲�������\n");
        rt_kprintf("�÷�: cardinal_trigger [amount] [light] [water]\n");
        return;
    }

    //���ò��Բ���
    g_cardinal_test_amount = amount;
    g_cardinal_test_L = light;
    g_cardinal_test_W = water;
    g_cardinal_test_flag = RT_TRUE;

    // �ͷ��ź������������߳�
    rt_sem_release(&cardinal_main_sem);
    rt_kprintf("[Cardinal] ����̨�ֶ����������߳�, amount=%d, light=%d, water=%d\n", amount, light, water);
}

//У׼

// ��WIFI���ã�У׼ʱ��
void cardinal_set_time(int hour, int minute)
{
    if (g_cardinal_time_stop)
    {
        rt_kprintf("[Cardinal] ʱ��������������У׼����\n");
        return;
    }
    g_cardinal_time.hour = hour;
    g_cardinal_time.minute = minute;
    rt_kprintf("[Cardinal] ʱ��У׼Ϊ %02d:%02d\n", hour, minute);
}

// ��WIFI���ã����¶�ʱ�����
// json: {"6":{"amount":5},"8":{"amount":1},"16":{"amount":1}}
// �ַ�������������������%f
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

// ��ȡ����Ӧ������ֵ
static int extract_int(const char *key, const char *json) {
    char *pos = strstr(json, key);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    return atoi(pos + 1);
}

// ��ȡ����Ӧ�ĸ���ֵ�����ַ�����Ƭ���ֶ�������
static float extract_float(const char *key, const char *json) {
    char *pos = strstr(json, key);
    if (!pos) return 0.0f;
    pos = strchr(pos, ':');
    if (!pos) return 0.0f;
    pos++;
    // �ҵ��������� , �� }
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

// ��ȡ�ƹ��ַ�����"0"��"1"��
static int extract_light(const char *key, const char *json) {
    char *pos = strstr(json, key);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    pos++; // skip :
    while (*pos && (*pos == ' ' || *pos == '"')) pos++; // �����ո������
    if (*pos >= '0' && *pos <= '9') // ����Ƿ�Ϊ����
        return *pos - '0';
    return 0;
}
// ������������ JSON �����������
// ������������ JSON �����������
void cardinal_update_tasks(const char *json) {
    // ���������ԭ����
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

        // ��λ��ǰСʱ��Ӧ�� JSON �Ӵ�
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
        rt_kprintf("[Cardinal] ���ݸ���");
    }
}

// ����̨�������ʱ��Ϊ10:00��ֹͣ����ʱ��
static void cardinal_stop_time(int argc, char **argv)
{
    g_cardinal_time_stop = RT_TRUE;
    g_cardinal_time.hour = 10;
    g_cardinal_time.minute = 0;
    rt_kprintf("[Cardinal] ʱ��������Ϊ10:00���������ٸ���ʱ��\n");
}
MSH_CMD_EXPORT(cardinal_stop_time, ����ʱ��Ϊ10:00��ֹͣ����ʱ��);

MSH_CMD_EXPORT(cardinal_trigger, �ֶ����������߳�: cardinal_trigger [amount] [light] [water]);

// ����̨�����ӡ��ʱ�����
static void cardinal_print_tasks(int argc, char **argv)
{
    rt_kprintf("Hour\tAmount(x100)\tLight\tWater\n");
    for (int i = 0; i < 24; i++)
    {
        if (g_cardinal_tasks[i].amount > 0 ||
            g_cardinal_tasks[i].light_mode > 0 ||
            g_cardinal_tasks[i].water_duration > 0)
        {
            // ��amount����100תΪ������ʾ������%f
            rt_kprintf("%i\t%d\t\t%i\t%i\n",
                i,
                (int)(g_cardinal_tasks[i].amount * 100),
                g_cardinal_tasks[i].light_mode,
                g_cardinal_tasks[i].water_duration);
        }
    }
}
MSH_CMD_EXPORT(cardinal_print_tasks, ��ӡ��ʱ�����);