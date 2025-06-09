#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

// ��������ṹ�壬Ϊ������ӵĹ���Ԥ��
typedef struct
{
    int angle; // ����Ƕȣ�0~180��
} servo_param_t;

servo_param_t g_servo_param = {100}; // Ĭ��90��

// PWM����
#define SERVO_PERIOD 1000 * 20 // 20ms���ڣ���λus
#define SERVO_MIN_PULSE 100 * 5   // 0�ȶ�Ӧ����0.5ms
#define SERVO_MAX_PULSE 100 * 25  // 180�ȶ�Ӧ����2.5ms

// �Ƕ�תռ�ձ�����
static uint16_t servo_angle_to_pulse(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    return (uint16_t)(SERVO_MIN_PULSE + (SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (angle / 180.0f));
}

// ��������߳�
static void servo_thread_entry(void *parameter)
{
    rt_kprintf("���Զ��2");
    int last_angle = -1;
    uint16_t period = SERVO_PERIOD;
    pwm_tim8_init(period, servo_angle_to_pulse(g_servo_param.angle));  // ��ʼ��һ��

    while (1)
    {
        
        if (g_servo_param.angle != last_angle)
        {
            uint16_t pulse = servo_angle_to_pulse(g_servo_param.angle);
            TIM_SetCompare1(TIM8, pulse);  // ? ֻ����ռ�ձȣ������� init
            last_angle = g_servo_param.angle;
            rt_kprintf("����Ƕ�: %d, ����: %d us\n", g_servo_param.angle, pulse);
        }

        rt_thread_mdelay(20);
    }
}


// PWM��ʼ��
void pwm_module_init(void)
{
    SystemCoreClockUpdate(); // ����ϵͳ����ʱ��
    rt_kprintf("���Զ��1");
    pwm_gpio_init();
    pwm_tim8_init(SERVO_PERIOD, servo_angle_to_pulse(g_servo_param.angle));
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
    {
        // rt_kprintf("����̴߳����ɹ�\n");
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("����̴߳���ʧ�ܣ�\n");
    }
}

// ����̨������ö���Ƕ�
static void servo_angle_cmd(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("�÷�: servo_angle �Ƕ�(0~180)\n");
        return;
    }
    int angle = atoi(argv[1]);
    if (angle < 0 || angle > 180)
    {
        rt_kprintf("�Ƕȷ�Χ0~180\n");
        return;
    }
    g_servo_param.angle = angle;
    rt_kprintf("���ö���Ƕ�: %d\n", angle);
}
#include <finsh.h>
MSH_CMD_EXPORT(servo_angle_cmd, ���ö���Ƕ� 0~180);

