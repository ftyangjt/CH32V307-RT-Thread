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
#define WIFI_PWR_PIN 11 //��֪�� @@@

wifi_param_t g_wifi_param = {0};
static char wifi_recv_buf[WIFI_RECV_BUF_SIZE];
static rt_size_t wifi_data_len = 0;
static struct rt_semaphore wifi_rx_sem;
static rt_device_t wifi_uart = RT_NULL;

// Flash �洢������ƫ�� @@@
#define WIFI_PARAM_PART_NAME "easyflash"
#define WIFI_PARAM_FLASH_OFFSET 0x0000

static void wifi_param_default(void)
{
    g_wifi_param.interval = 0;
    g_wifi_param.amount = 0;
    g_wifi_param.blink = 0;
}

//û��FAL֧�֣���������� @@@
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
        // ��У��
        if (g_wifi_param.interval < 0.01f || g_wifi_param.interval > 10000.0f)
            wifi_param_default();
    }
    else
#endif
    {
        wifi_param_default();
    }
}


//����json
static void wifi_parse_json(const char *json)
{
    // ���������AAAE
    if (strcmp(json, "AAA") == 0)
    {
        cardinal_stop_time(0, RT_NULL);
        return;
    }

    // ����Ƿ�Ϊʱ��У׼��ʽ��[MM:SS]
    if (json[0] == '[')
    {
        int mm, ss;
        if (sscanf(json, "[%2d:%2d]", &mm, &ss) == 2)
        {
            // ���� cardinal_set_time ����ʱ�������У׼
            cardinal_set_time(mm, ss);
            return;
        }
    }

    // ����Ƿ�Ϊʱ��У׼��ʽ��[MM:SS]
    if (json[0] == 'L')
    {
        int n;
        if (sscanf(json, "L%d", &n) == 1)
        {
            rainBowType = n;
            return;
        }
    }

    // ����Ƿ�Ϊ��ʱ�����JSON��ʽ������Сʱ������Ϊ����amount�ֶ�
    if (json[0] == '{' && strstr(json, "\"amount\":"))
    {
        // ����Cardinalģ����������º���
        cardinal_update_tasks(json);
        return;
    }

    // �������ƥ�䣬���δ֪JSON
    rt_kprintf("�յ�δ֪JSON: %s\n", json);
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
        // �����ȴ��ź��������жϸ�����
        rt_sem_take(&wifi_rx_sem, RT_WAITING_FOREVER);

        // ��ȡ��������
        while (rt_device_read(wifi_uart, 0, &ch, 1) == 1)
        {
            if (ch == 'E')
            {
                // ����E������ǰ������
                wifi_recv_buf[wifi_data_len] = '\0';
                rt_kprintf("ת����Ϣ: %s\n", wifi_recv_buf);
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
                wifi_data_len = 0; // ��ֹ���
                wifi_recv_buf[0] = '\0';
            }
        }
    }
}


void wifi_power_ctrl(rt_bool_t on)
{
    rt_pin_write(WIFI_PWR_PIN, on ? PIN_HIGH : PIN_LOW);
    rt_kprintf("WiFiģ���Դ: %s\n", on ? "ON" : "OFF");
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

    // ��ʼ����Դ��������
    rt_pin_mode(WIFI_PWR_PIN, PIN_MODE_OUTPUT);
    wifi_power_ctrl(RT_TRUE); // Ĭ���ϵ�
}



