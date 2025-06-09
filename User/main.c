/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2020/04/30
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>
#include "drivers/pin.h"
#include "dht11.h"

#include <drv_gpio.h> // 确保包含了 RT-Thread 的 GPIO 驱动支持
#include <string.h>
#include "ATK.h"
#include <stdlib.h>
#include "WIFI.h"
#include "PWM.h"   // 新增，包含PWM头文件
#include "drv_pwm.h"


/* Global typedef */

/* Global define */

/* LED0 is driven by the pin driver interface of rt  */
#define LED0_PIN 10  // 使用 PC0 引脚

/* Global Variable */

/*********************************************************************
 * @fn      LED1_BLINK_INIT
 *
 * @brief   LED1 directly calls the underlying driver
 *
 * @return  none
 */
/*********************************************************************
 * @fn      main
 *
 * @brief   main is just one of the threads, in addition to tshell,idle
 * This main is just a flashing LED, the main thread is registered in 
 * rtthread_startup, tshell uses the serial port to receive interrupts, 
 * and the interrupt stack and thread stack are used separately.
 *
 * @return  none
 */
 
 //测试变量
#define ATK_UART_NAME "uart2"
extern rt_device_t atk_uart;
static char recv_buf[128];
static int var1 = 0;



/*********************************************************************
 * @fn      led
 *
 * @brief   Test using the driver interface to operate the I/O port
 *
 * @return  none
 */

 void read_dht11_sample(void)
{
    uint8_t temp, humi;
    if (dht11_read_data(&temp, &humi) == 0)
    {
        rt_kprintf("Temp: %d °C, Humi: %d %%\n", temp, humi);
    }
    else
    {
        rt_kprintf("DHT11 read failed\n");
    }
}

int led(void)
{
    rt_uint8_t count;

    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    printf("led_SP:%08x\r\n",__get_SP());
    for(count = 0 ; count < 10 ;count++)
    {
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_kprintf("led on, count : %d\r\n", count);
        rt_thread_mdelay(500);

        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_kprintf("led off\r\n");
        rt_thread_mdelay(500);
    }
    return 0;
}


static rt_err_t atk_uart_rx(rt_device_t dev, rt_size_t size)
{
    // 数据接收回调
    int len = rt_device_read(dev, 0, recv_buf, sizeof(recv_buf) - 1);
    recv_buf[len] = '\0';

    if (strstr(recv_buf, "GET /control?var1="))
    {
        char *p = strstr(recv_buf, "var1=");
        if (p)
        {
            int val = atoi(p + 5);
            var1 = val;
            rt_kprintf("变量 var1 被设置为：%d\n", var1);
        }
    }

    return RT_EOK;
}

MSH_CMD_EXPORT(led,  led sample by using I/O drivers);
MSH_CMD_EXPORT(read_dht11_sample, read DHT11 sensor);
MSH_CMD_EXPORT(send_at_cmd, 发送 AT 指令到 WiFi 模块);

int main(void)
{
    rt_kprintf("\r\n MCU: CH32V307\r\n");
    SystemCoreClockUpdate();
    rt_kprintf(" SysClk: %dHz\r\n", SystemCoreClock);
    rt_kprintf(" ChipID: %08x\r\n", DBGMCU_GetCHIPID());
    rt_kprintf(" www.wch.cn\r\n");

    // atk8266_wifi_ap_web_init();  //旧WiFi模块初始化
    wifi_module_init();             
    pwm_module_init();   
       

    while (1)
    {
        rt_kprintf("主循环运行中...\n");
        rt_thread_mdelay(3000);
    }
}
