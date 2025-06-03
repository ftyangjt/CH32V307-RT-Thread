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
    float interval = 0, amount = 0;
    int blink = 0;

    //%f������ �������ų����²�
    //Ŀǰ����ֵ״��{"interval":12.5,"amount":1.0,"blink":3}
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
    //%f�����ã�����
    rt_kprintf("WiFi��������: interval=%d.%d, amount=%d.%d, blink=%d\n",
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
                    //���� JSON ��ʼ
                    char *start = strchr(wifi_recv_buf, '{');
                    if (start)
                    {
                         rt_kprintf("�յ�ԭʼ����: %s\n", start); // ���Դ�ӡ
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

    // ��ʼ����Դ��������
    rt_pin_mode(WIFI_PWR_PIN, PIN_MODE_OUTPUT);
    wifi_power_ctrl(RT_TRUE); // Ĭ���ϵ�
}

void wifi_power_ctrl(rt_bool_t on)
{
    rt_pin_write(WIFI_PWR_PIN, on ? PIN_HIGH : PIN_LOW);
    rt_kprintf("WiFiģ���Դ: %s\n", on ? "ON" : "OFF");
}

// ����̨���� @@@
static void wifi_power_cmd(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("�÷�: wifi_power on/off\n");
        return;
    }
    if (!strcmp(argv[1], "on"))
        wifi_power_ctrl(RT_TRUE);
    else if (!strcmp(argv[1], "off"))
        wifi_power_ctrl(RT_FALSE);
    else
        rt_kprintf("��������: ������ on �� off\n");
}
MSH_CMD_EXPORT(wifi_power_cmd, ����WiFiģ���Դ����);

