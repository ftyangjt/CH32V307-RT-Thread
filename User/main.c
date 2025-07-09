/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : ChatGPT
* Version            : V2.0.0
* Date               : 2025/05/27
* Description        : CH32V307 + RT-Thread ������
*********************************************************************************/
// ����RTT�ļ�
#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>

// ��������ģ��ͷ�ļ�
#include "ws2812b/ws2812.h"
#include "ws2812b/rainbow.h"

// ������Ļģ��ͷ�ļ�
#include "BSP/LCD/lcd.h"
#include "screen/screen.h"

// ����WIFIģ��ͷ�ļ�
#include "WIFI.h"
#include "PWM.h"
#include "drv_pwm.h"
#include "shell.h"
#include "Cardinal.h"
#include "PUMP.h"


// �����¶�ͷ�ļ�
#include "BSP/ADC/temp_adc.h"

int main(void)
{
    SystemCoreClockUpdate();

    // ��ʼ�� Finsh Shell
    finsh_system_init();
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);

    rt_device_t console = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (console)
    {
        rt_device_open(console, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }

    // ��ʼ����Ļ
    lcd_init();
    ws2812_init(); //led
    
    // ��ʼ��WIFI�͵��
    cardinal_module_init(); //�����߳�
    pump_thread_init();
    pwm_module_init();  
    wifi_module_init(); 
    breathing_start(); //�������߳�
    rainbow_init();
    rainbow_start(1);

    
    // ��ʼ���¶���ʾ
    adc_temperature_init();

    // ��ʼ����Ļģ�鲢�������UI��ʾ�߳�
    screen_init();

    // ��ѭ��
    while(1)
    {
        rt_thread_mdelay(1000);
        rt_kprintf("���߳�\n");
        // ��ѭ�����ֿ��У��������߳����
    }
}