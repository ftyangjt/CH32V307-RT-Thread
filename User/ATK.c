#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

#define ATK_UART_NAME "uart2"
#define ATK_RECV_BUF_SIZE 512

rt_device_t atk_uart = RT_NULL;
static char atk_recv_buf[ATK_RECV_BUF_SIZE];
static rt_size_t atk_data_len = 0;
struct rt_semaphore atk_rx_sem;

/* 串口接收中断回调：仅发信号通知 */
static rt_err_t atk_uart_rx_callback(rt_device_t dev, rt_size_t size)
{
    
    rt_sem_release(&atk_rx_sem);  // 通知接收线程
    
    return RT_EOK;
}

/* 接收数据处理线程：从串口缓冲中读取字符并拼接、打印 */
static void atk_recv_thread_entry(void *parameter)
{
    char ch;
    
    while (1)
    {
        rt_sem_take(&atk_rx_sem, RT_WAITING_FOREVER);
        //rt_kprintf("[debug] atk_rx_callback 信号触发\n");

        while (rt_device_read(atk_uart, 0, &ch, 1) == 1)
        {
            if (atk_data_len < ATK_RECV_BUF_SIZE - 1)
            {
                atk_recv_buf[atk_data_len++] = ch;
                atk_recv_buf[atk_data_len] = '\0';

                if (atk_data_len >= 2 &&
                    atk_recv_buf[atk_data_len - 2] == '\r' &&
                    atk_recv_buf[atk_data_len - 1] == '\n')
                {
                    atk_recv_buf[atk_data_len - 2] = '\0';
                    rt_kprintf("Recv: %s\n", atk_recv_buf);
                    atk_data_len = 0;
                }
            }
            else
            {
                rt_kprintf("Recv buffer overflow!\n");
                atk_data_len = 0;
            }
        }
    }
}

/* 发送一条 AT 指令 */
void send_at_cmd(const char *cmd)
{
    rt_device_write(atk_uart, 0, cmd, rt_strlen(cmd));
    rt_kprintf("Send: %s", cmd); // 不加 \n，因为 cmd 已包含 \r\n
    rt_thread_mdelay(1500); // 等待回应
}

/* 初始化 UART2 + 创建接收线程 + 启动热点与 Web 页面 */
void atk8266_wifi_ap_web_init(void)
{
    atk_uart = rt_device_find(ATK_UART_NAME);
    if (!atk_uart)
    {
        rt_kprintf("ATK UART NOT FOUND!\n");
        return;
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_115200;

    rt_sem_init(&atk_rx_sem, "atksem", 0, RT_IPC_FLAG_FIFO);
    rt_device_control(atk_uart, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_open(atk_uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(atk_uart, atk_uart_rx_callback);



    rt_thread_t tid = rt_thread_create("atkrecv",
                                       atk_recv_thread_entry,
                                       RT_NULL,
                                       1024, 5, 10);
    if (tid)
    {
        rt_kprintf("[debug] 接收线程已创建并启动\n");
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("[error] 接收线程创建失败！\n");
    }


    // 初始化 ESP8266 模块
    send_at_cmd("ATE0\r\n");                     // 关闭回显
    send_at_cmd("AT\r\n");
    send_at_cmd("AT+CWMODE=2\r\n");              // AP 模式
    send_at_cmd("AT+CWSAP=\"AAAA\",\"12345678\",5,3\r\n");  // 设置热点
    send_at_cmd("AT+CIPMUX=1\r\n");              // 多连接
    send_at_cmd("AT+CIPSERVER=1,80\r\n");        // 启动 Web 服务器
    send_at_cmd("AT+CAPTIVE=1\r\n");
}
