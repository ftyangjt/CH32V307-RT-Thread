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
    rt_hw_us_delay(us); // RT-Thread ��ȷ��ʱ����
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
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // ��������
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

//���Ƕ��ӳٺ����ĸ���ʵ�֣����ź�RTT�ٷ������䡣��÷��� board.c ��
void rt_hw_us_delay(rt_uint32_t us)
{
    // 144��ѭ����Լ1΢�루��Ҫ��ʵ��ƽ̨����У׼��
    while(us--)
    {
        for (int i = 0; i < 35; i++) // ���Թ���
        {
            __asm volatile("nop");
        }
    }
}

int dht11_read_data(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t data[5] = {0};
    rt_uint8_t i, j;

    // ���Ϳ�ʼ�ź�
    gpio_mode_output();
    DHT11_PIN_LOW();
    rt_thread_mdelay(20);      // �������� 18ms
    DHT11_PIN_HIGH();
    delay_us(30);              // �ȴ� 20~40us


    gpio_mode_input();

    if (DHT11_READ_PIN() == 1)
    {
        rt_kprintf("DHT11 δ��Ӧ������ʼ��Ϊ�ߣ�\n");
        return -1;
    }
    while (DHT11_READ_PIN() == 0);
    rt_kprintf("DHT11 ������Ӧ OK\n");
    while (DHT11_READ_PIN() == 1);
    rt_kprintf("DHT11 ׼����������\n");

    for (j = 0; j < 5; j++)
    {
        for (i = 0; i < 8; i++)
        {
            while (DHT11_READ_PIN() == 0); // �ȴ�����
            delay_us(40); // ��ȡ��λֵ

            if (DHT11_READ_PIN() == 1)
                data[j] |= (1 << (7 - i));

            while (DHT11_READ_PIN() == 1); // �ȴ�����
        }
    }

    if (data[0] + data[1] + data[2] + data[3] == data[4])
    {
        *humidity = data[0];
        *temperature = data[2];
        return 0;
    }

    return -2; // У��ʧ��
}
