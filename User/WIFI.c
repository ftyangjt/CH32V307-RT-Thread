#include "WIFI.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <finsh.h>
#include "drivers/pin.h"

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

    //%f不可用 ！！！才出此下策
    //目前返回值状如{"interval":12.5,"amount":1.0,"blink":3}
   char buf[32];

    // interval
    char *p = strstr(json, "\"interval\":");
    if (p)
    {
        p += strlen("\"interval\":");
        sscanf(p, "%31[^,}]", buf);
        interval = (float)atof(buf);
    }

    // amount
    p = strstr(json, "\"amount\":");
    if (p)
    {
        p += strlen("\"amount\":");
        sscanf(p, "%31[^,}]", buf);
        amount = (float)atof(buf);
    }

    // blink
    p = strstr(json, "\"blink\":");
    if (p)
    {
        p += strlen("\"blink\":");
        sscanf(p, "%31[^,}]", buf);
        blink = atoi(buf);
    }

    g_wifi_param.interval = interval;
    g_wifi_param.amount = amount;
    g_wifi_param.blink = blink;
    wifi_param_save();
    //%f不可用！！！
    rt_kprintf("WiFi参数更新: interval=%d.%d, amount=%d.%d, blink=%d\n",
        (int)interval, (int)(interval*100)%100,
        (int)amount, (int)(amount*100)%100,
        blink);
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
                    //查找 JSON 起始
                    char *start = strchr(wifi_recv_buf, '{');
                    if (start)
                    {
                         rt_kprintf("收到原始数据: %s\n", start); // 调试打印
                        wifi_parse_json(start);
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

    rt_thread_t tid = rt_thread_create("wifirecv", wifi_recv_thread_entry, RT_NULL, 1024, 8, 10);
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

