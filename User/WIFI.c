#include "WIFI.h"
#include "PWM.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <finsh.h>
#include "drivers/pin.h"
#include "Cardinal.h"
#include "ws2812b/rainbow.h"

#define WIFI_UART_NAME "uart2"
#define WIFI_RECV_BUF_SIZE 1024
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
    // 新增：检测AAAE
    if (strcmp(json, "AAA") == 0)
    {
        cardinal_stop_time(0, RT_NULL);
        return;
    }

    // 检测是否为时间校准格式：[MM:SS]
    if (json[0] == '[')
    {
        int mm, ss;
        if (sscanf(json, "[%2d:%2d]", &mm, &ss) == 2)
        {
            // 调用 cardinal_set_time 进行时间解析和校准
            cardinal_set_time(mm, ss);
            return;
        }
    }

    // 检测是否为时间校准格式：[MM:SS]
    if (json[0] == 'L')
    {
        int n;
        if (sscanf(json, "L%d", &n) == 1)
        {
            rainBowType = n;
            return;
        }
    }

    // 检测是否为定时任务表JSON格式：包含小时数字作为键和amount字段
    if (json[0] == '{' && strstr(json, "\"amount\":"))
    {
        // 调用Cardinal模块的任务表更新函数
        cardinal_update_tasks(json);
        return;
    }

    // 如果都不匹配，输出未知JSON
    rt_kprintf("收到未知JSON: %s\n", json);
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
        // 阻塞等待信号量（由中断给出）
        rt_sem_take(&wifi_rx_sem, RT_WAITING_FOREVER);

        // 读取串口数据
        while (rt_device_read(wifi_uart, 0, &ch, 1) == 1)
        {
            if (ch == 'E')
            {
                // 读到E，处理前面内容
                wifi_recv_buf[wifi_data_len] = '\0';
                rt_kprintf("转发消息: %s\n", wifi_recv_buf);
                wifi_parse_json(wifi_recv_buf);
                wifi_data_len = 0;
                wifi_recv_buf[0] = '\0';
            }
            else if (wifi_data_len < WIFI_RECV_BUF_SIZE - 1)
            {
                wifi_recv_buf[wifi_data_len++] = ch;
                wifi_recv_buf[wifi_data_len] = '\0';
            }
            else
            {
                wifi_data_len = 0; // 防止溢出
                wifi_recv_buf[0] = '\0';
            }
        }
    }
}


void wifi_power_ctrl(rt_bool_t on)
{
    rt_pin_write(WIFI_PWR_PIN, on ? PIN_HIGH : PIN_LOW);
    rt_kprintf("WiFi模块电源: %s\n", on ? "ON" : "OFF");
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

    rt_thread_t tid = rt_thread_create("wifirecv", wifi_recv_thread_entry, RT_NULL, 4096, 5, 10);
    if (tid)
        rt_thread_startup(tid);

    // 初始化电源控制引脚
    rt_pin_mode(WIFI_PWR_PIN, PIN_MODE_OUTPUT);
    wifi_power_ctrl(RT_TRUE); // 默认上电
}



