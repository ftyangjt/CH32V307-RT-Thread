#include <rtthread.h>
#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"

#include "rthw.h"

#define DHT11_GPIO_PORT GPIOA
#define DHT11_GPIO_PIN  GPIO_Pin_0

#define DHT11_PIN_HIGH() GPIO_WriteBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN, Bit_SET)
#define DHT11_PIN_LOW()  GPIO_WriteBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN, Bit_RESET)
#define DHT11_READ_PIN() GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

static void delay_us(rt_uint32_t us)
{
    rt_hw_us_delay(us); // RT-Thread 精确延时函数
}

static void gpio_mode_output(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

static void gpio_mode_input(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

//这是对延迟函数的个人实现，很遗憾RTT官方无适配。最好放在 board.c 里
void rt_hw_us_delay(rt_uint32_t us)
{
    // 144次循环大约1微秒（需要你实际平台测试校准）
    while(us--)
    {
        for (int i = 0; i < 35; i++) // 粗略估算
        {
            __asm volatile("nop");
        }
    }
}

int dht11_read_data(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t data[5] = {0};
    rt_uint8_t i, j;

    // 发送开始信号
    gpio_mode_output();
    DHT11_PIN_LOW();
    rt_thread_mdelay(20);      // 拉低至少 18ms
    DHT11_PIN_HIGH();
    delay_us(30);              // 等待 20~40us


    gpio_mode_input();

    if (DHT11_READ_PIN() == 1)
    {
        rt_kprintf("DHT11 未响应（引脚始终为高）\n");
        return -1;
    }
    while (DHT11_READ_PIN() == 0);
    rt_kprintf("DHT11 拉高响应 OK\n");
    while (DHT11_READ_PIN() == 1);
    rt_kprintf("DHT11 准备发送数据\n");

    for (j = 0; j < 5; j++)
    {
        for (i = 0; i < 8; i++)
        {
            while (DHT11_READ_PIN() == 0); // 等待拉高
            delay_us(40); // 读取中位值

            if (DHT11_READ_PIN() == 1)
                data[j] |= (1 << (7 - i));

            while (DHT11_READ_PIN() == 1); // 等待拉低
        }
    }

    if (data[0] + data[1] + data[2] + data[3] == data[4])
    {
        *humidity = data[0];
        *temperature = data[2];
        return 0;
    }

    return -2; // 校验失败
}
