#include "WIFI.h"
#include "PWM.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <finsh.h>
#include "drivers/pin.h"
#include "Cardinal.h" // 新增

#define WIFI_UART_NAME "uart2"
#define WIFI_RECV_BUF_SIZE 128
#define WIFI_PWR_PIN 11 //不知道 @@@

wifi_param_t g_wifi_param = {0};
static char wifi_recv_buf[WIFI_RECV_BUF_SIZE];
static rt_size_t wifi_data_len = 0;
static struct rt_semaphore wifi_rx_sem;
static rt_device_t wifi_uart = RT_NULL;

// Flash 存储区名和偏移 @@@
#define WIFI_PARAM_PART_NAME "easyflash"
#define WIFI_PARAM_FLASH_OFFSET 0x0000

static void wifi_param_default(void)
{
    g_wifi_param.interval = 0;
    g_wifi_param.amount = 0;
    g_wifi_param.blink = 0;
}

//没有FAL支持？解决方案？ @@@
void wifi_param_save(void)
{
#ifdef PKG_USING_FAL
    struct fal_partition *part = fal_partition_find(WIFI_PARAM_PART_NAME);
    if (part)
    {
        fal_partition_erase(part, WIFI_PARAM_FLASH_OFFSET, sizeof(g_wifi_param));
        fal_partition_write(part, WIFI_PARAM_FLASH_OFFSET, (uint8_t *)&g_wifi_param, sizeof(g_wifi_param));
    }
#endif
}

void wifi_param_load(void)
{
#ifdef PKG_USING_FAL
    struct fal_partition *part = fal_partition_find(WIFI_PARAM_PART_NAME);
    if (part)
    {
        fal_partition_read(part, WIFI_PARAM_FLASH_OFFSET, (uint8_t *)&g_wifi_param, sizeof(g_wifi_param));
        // 简单校验
        if (g_wifi_param.interval < 0.01f || g_wifi_param.interval > 10000.0f)
            wifi_param_default();
    }
    else
#endif
    {
        wifi_param_default();
    }
}

//解析json
static void wifi_parse_json(const char *json)
{
    float interval = 0, amount = 0;
    int blink = 0;
    rt_bool_t set_flag = RT_FALSE;
    rt_bool_t calibrate_flag = RT_FALSE;
    char time_str[32] = {0};

    //%f不可用 ！！！才出此下策
    //目前返回值状如{"interval":12.5,"amount":1.0,"blink":3}
    char buf[32];

    // interval
    char *p = strstr(json, "\"interval\":");


    // time
    p = strstr(json, "\"time\":");
    if (p)
    {
        calibrate_flag = RT_TRUE;
        p += strlen("\"time\":");
        // 跳过可能的空格和引号
        while (*p == ' ' || *p == '\"') p++;
        sscanf(p, "%31[^\"]", time_str);
        // 新增：调用Cardinal校准时间
        int hour = 0, minute = 0;
        if (sscanf(time_str, "%d:%d", &hour, &minute) == 2)
        {
            cardinal_set_time(hour, minute);
        }
    }

    //定时任务表JSON（{"6":{"amount":0.1,"light":"1","water":0,"minute":0},"8":{"amount":1,"light":"0","water":5,"minute":0},"16":{"amount":3.7,"light":"1","water":0,"minute":0}}）
    if (json[0] == '{' && strstr(json, "\"amount\":"))
    {
        // 新增：解析新版定时任务表
        // 逐小时查找
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
                    // 跳过可能的引号
                    while (*q == ' ' || *q == '\"') q++;
                    sscanf(q, "%7[^\",]", buf); 
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
                // 更新任务表
                if (hour >= 0 && hour < 24)
                {
                    extern cardinal_task_t g_cardinal_tasks[24];
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
        rt_kprintf("定时任务表已更新\n");
    }
    if (calibrate_flag)
    {
        rt_kprintf("收到时间校准: time=%s\n", time_str);
    }
    if (!set_flag && !calibrate_flag && !(json[0] == '{' && strstr(json, "\"amount\":")))
    {
        rt_kprintf("收到未知JSON: %s\n", json);
    }
}

static rt_err_t wifi_uart_rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&wifi_rx_sem);
    return RT_EOK;
}

static void wifi_recv_thread_entry(void *parameter)
{
    char ch;
    while (1)
    {
        rt_sem_take(&wifi_rx_sem, RT_WAITING_FOREVER);
        while (rt_device_read(wifi_uart, 0, &ch, 1) == 1)
        {
            if (wifi_data_len < WIFI_RECV_BUF_SIZE - 1)
            {
                wifi_recv_buf[wifi_data_len++] = ch;
                wifi_recv_buf[wifi_data_len] = '\0';
                if (ch == '}')
                {
                    // 查找 JSON 起始
                    char *start = strchr(wifi_recv_buf, '{');
                    if (start)
                    {
                        // 检查括号配对，确保是完整JSON
                        int brace = 0;
                        char *p = start;
                        while (*p)
                        {
                            if (*p == '{') brace++;
                            else if (*p == '}') brace--;
                            if (brace == 0) break;
                            p++;
                        }
                        if (brace == 0 && *p == '}')
                        {
                            int json_len = p - start + 1;
                            char json_buf[WIFI_RECV_BUF_SIZE] = {0};
                            strncpy(json_buf, start, json_len);
                            json_buf[json_len] = '\0';
                            // rt_kprintf("收到原始数据: %s\n", json_buf); // 只回显完整JSON
                            wifi_parse_json(json_buf);
                        }
                    }
                    wifi_data_len = 0;
                }
            }
            else
            {
                wifi_data_len = 0;
            }
        }
    }
}

void wifi_module_init(void)
{
    wifi_param_load();

    wifi_uart = rt_device_find(WIFI_UART_NAME);
    if (!wifi_uart)
    {
        rt_kprintf("WiFi UART NOT FOUND!\n");
        return;
    }
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_115200;
    rt_sem_init(&wifi_rx_sem, "wifisem", 0, RT_IPC_FLAG_FIFO);
    rt_device_control(wifi_uart, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_open(wifi_uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(wifi_uart, wifi_uart_rx_callback);

    rt_thread_t tid = rt_thread_create("wifirecv", wifi_recv_thread_entry, RT_NULL, 1024, 5, 10);
    if (tid)
        rt_thread_startup(tid);

    // 初始化电源控制引脚
    rt_pin_mode(WIFI_PWR_PIN, PIN_MODE_OUTPUT);
    wifi_power_ctrl(RT_TRUE); // 默认上电
}

void wifi_power_ctrl(rt_bool_t on)
{
    rt_pin_write(WIFI_PWR_PIN, on ? PIN_HIGH : PIN_LOW);
    rt_kprintf("WiFi模块电源: %s\n", on ? "ON" : "OFF");
}

// 控制台命令 @@@
static void wifi_power_cmd(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("用法: wifi_power on/off\n");
        return;
    }
    if (!strcmp(argv[1], "on"))
        wifi_power_ctrl(RT_TRUE);
    else if (!strcmp(argv[1], "off"))
        wifi_power_ctrl(RT_FALSE);
    else
        rt_kprintf("参数错误: 请输入 on 或 off\n");
}
MSH_CMD_EXPORT(wifi_power_cmd, 控制WiFi模块电源开关);

