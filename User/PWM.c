#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

// ��������ṹ�壺�ٶȣ�-100~100��������ʱ�䣨�룩
typedef struct
{
    int speed;
    int duration_sec;
} servo_param_t;

servo_param_t g_servo_param = {10, 999999}; // Ĭ��ֹͣ

#define SERVO_PERIOD      20000  // 20ms���ڣ���λus
#define SERVO_MIN_PULSE    500   // 0.5ms�����ת
#define SERVO_STOP_PULSE  1500   // 1.5ms��ֹͣ
#define SERVO_MAX_PULSE   2500   // 2.5ms�������ת

static uint16_t servo_speed_to_pulse(int speed)
{
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    if (speed == 0) return SERVO_STOP_PULSE;
    if (speed > 0)
        return SERVO_STOP_PULSE + (SERVO_MAX_PULSE - SERVO_STOP_PULSE) * speed / 100;
    else
        return SERVO_STOP_PULSE - (SERVO_STOP_PULSE - SERVO_MIN_PULSE) * (-speed) / 100;
}

// ��������߳�
static void servo_thread_entry(void *parameter)
{
    int last_speed = -999;
    pwm_gpio_init_10();
    pwm_tim10_init(SERVO_PERIOD, servo_speed_to_pulse(0));
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    TIM_Cmd(TIM10, ENABLE);

    while (1)
    {
        if (g_servo_param.speed != 0 && g_servo_param.duration_sec > 0)
        {
            rt_kprintf("����������ٶ�: %d������: %d ��\n", g_servo_param.speed, g_servo_param.duration_sec);
            TIM_SetCompare1(TIM10, servo_speed_to_pulse(g_servo_param.speed));
            last_speed = g_servo_param.speed;
            for (int i = 0; i < g_servo_param.duration_sec * 50; i++) // 20ms*50=1s
            {
                rt_thread_mdelay(20);
                // ���ڼ��ٶȱ��ⲿ������ģ�������Ӧ
                if (g_servo_param.speed != last_speed)
                {
                    TIM_SetCompare1(TIM10, servo_speed_to_pulse(g_servo_param.speed));
                    last_speed = g_servo_param.speed;
                }
            }
            // ֹͣ
            TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
            g_servo_param.speed = 0;
            g_servo_param.duration_sec = 0;
            rt_kprintf("���������ɣ���ֹͣ��\n");
        }
        else
        {
            // ���ٶȱ仯����������PWM
            if (last_speed != g_servo_param.speed)
            {
                TIM_SetCompare1(TIM10, servo_speed_to_pulse(g_servo_param.speed));
                last_speed = g_servo_param.speed;
            }
            rt_thread_mdelay(20);
        }
    }
}

// PWM��ʼ��
void pwm_module_init(void)
{
    SystemCoreClockUpdate();
    rt_kprintf("PWM���ģ���ʼ��\n");
    pwm_gpio_init_10();
    pwm_tim10_init(SERVO_PERIOD, servo_speed_to_pulse(0));
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    TIM_Cmd(TIM10, ENABLE);
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
        rt_thread_startup(tid);
    else
        rt_kprintf("����̴߳���ʧ�ܣ�\n");
}

// ����̨������ö���ٶȺ���ת����
static void servo_run_cmd(int argc, char **argv)
{
    if (argc != 3)
    {
        rt_kprintf("�÷�: servo_run �ٶ�(-100~100) ����\n");
        return;
    }
    int speed = atoi(argv[1]);
    int duration = atoi(argv[2]);
    if (speed < -100 || speed > 100)
    {
        rt_kprintf("�ٶȷ�Χ-100~100\n");
        return;
    }
    if (duration <= 0)
    {
        rt_kprintf("����ʱ�������0��\n");
        return;
    }
    g_servo_param.speed = speed;
    g_servo_param.duration_sec = duration;
    rt_kprintf("���ö���ٶ�: %d, ����: %d ��\n", speed, duration);
}
#include <finsh.h>
MSH_CMD_EXPORT(servo_run_cmd, ���ö���ٶȺ���ת����: servo_run �ٶ� ����);

// ����̨�������ֹͣ���
static void servo_stop_cmd(int argc, char **argv)
{
    g_servo_param.speed = 0;
    g_servo_param.duration_sec = 0;
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    rt_kprintf("�����ֹͣ\n");
}
MSH_CMD_EXPORT(servo_stop_cmd, ����ֹͣ���);

