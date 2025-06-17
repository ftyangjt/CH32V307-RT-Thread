/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : ChatGPT
* Version            : V2.0.0
* Date               : 2025/05/27
* Description        : CH32V307 + RT-Thread ������
*********************************************************************************/

#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>
#include "drivers/pin.h"

// ����ģ��ͷ�ļ�
#include "ws2812b/ws2812.h"
#include "ws2812b/rainbow.h"

// ������Ļͷ�ļ�
#include "lcd/lcd_rt.h"

// �Զ����ַ���ת��������
static int str_to_int(const char* str)
{
    int result = 0;
    int sign = 1;
    
    // �������
    if(*str == '-') {
        sign = -1;
        str++;
    } else if(*str == '+') {
        str++;
    }
    
    // ��������
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

// MSH��������ʺ�Ч��
static int cmd_rainbow(int argc, char **argv)
{
    int speed = 3;  // Ĭ���ٶȼ���
    
    if(argc > 1) {
        speed = str_to_int(argv[1]);
    }
    
    rainbow_start(speed);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow, start rainbow effect [speed 1-5]);

// MSH���ֹͣ�ʺ�Ч��
static int cmd_rainbow_stop(int argc, char **argv)
{
    rainbow_stop();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_stop, stop rainbow effect);

// MSH������òʺ�Ч������
static int cmd_brightness(int argc, char **argv)
{
    int brightness = 30;  // Ĭ������30%
    
    if(argc > 1) {
        brightness = str_to_int(argv[1]);
        if(brightness < 0) brightness = 0;
        if(brightness > 100) brightness = 100;
    }
    
    rainbow_set_brightness(brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_brightness, set rainbow brightness [0-100]);

// LCD��ʾ�߳�
static void lcd_show_entry(void *parameter)
{
     lcd_hw_init();
    // rt_kprintf("lcd_show_entry run!\n");
    // lcd_hw_clear(0xFFFF); // ��ɫ����
    // lcd_hw_show_string(10, 10, "Hello RT-Thread!", 0xF800); // ��ɫ
    // lcd_hw_show_string(10, 40, "LCD Test", 0x001F);         // ��ɫ
    // lcd_hw_show_string(10, 70, "����ԭ��", 0x07E0);          // ��ɫ

    // �߳�������ɺ�����Լ�
    rt_thread_suspend(rt_thread_self());
    rt_schedule();
}

int main(void)
{
    rt_kprintf("\r\n");
    rt_kprintf("******************************************************\r\n");
    rt_kprintf("* MCU: CH32V307                                      *\r\n");
    rt_kprintf("* System: V2.0.0                                     *\r\n");
    rt_kprintf("* Date: 2025/05/27                                   *\r\n");
    rt_kprintf("******************************************************\r\n");
    
    SystemCoreClockUpdate();
    rt_kprintf("SysClk: %dHz\r\n", SystemCoreClock);
    rt_kprintf("ChipID: %08x\r\n", DBGMCU_GetCHIPID());
    


    // ����LCD��ʾ�߳�
    rt_thread_t tid = rt_thread_create("lcd_show",
                                       lcd_show_entry,
                                       RT_NULL,
                                       1024,
                                       10,
                                       10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        rt_kprintf("RT_NULL!!");
    
    // ��ʼ����ģ��
    ws2812_init();
    rainbow_init();
    // �����ʺ�Ч��
    rainbow_start(3);  // �ٶȼ���3

    // ��ѭ��
    while(1)
    {
        rt_thread_mdelay(1000);
        // ��ѭ�����ֿ��У��������߳����
    }
}