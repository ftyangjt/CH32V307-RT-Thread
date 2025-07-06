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

// �����¶�ͷ�ļ�
#include "BSP/ADC/temp_adc.h"

int main(void)
{
    SystemCoreClockUpdate();

    // ��ʼ����Ļ
    lcd_init();

    // ��ʼ���ʺ�ƹ�ģ��
    ws2812_init();
    rainbow_init();
    rainbow_start(3);  // �����ʺ�Ч�����ٶȼ���3

    // ��ʼ��WIFI�͵��
    pwm_module_init();  
    wifi_module_init(); 
    cardinal_module_init(); //�����߳�
    
    // ��ʼ���¶���ʾ
    adc_temperature_init();

    // ��ʼ����Ļģ�鲢�������UI��ʾ�߳�
    screen_init();

    // ��ѭ��
    while(1)
    {
        rt_thread_mdelay(1000);
        // ��ѭ�����ֿ��У��������߳����
    }
}