#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

// ������ת��������ṹ��
typedef struct
{
    int speed; // ����ٶȣ�-100~100��0Ϊֹͣ��
} servo_param_t;

servo_param_t g_servo_param = {0}; // Ĭ��ֹͣ

// PWM����
#define SERVO_PERIOD 20000      // 20ms���ڣ���λus
#define SERVO_MIN_PULSE 500     // 0.5ms�����ת
#define SERVO_STOP_PULSE 1500   // 1.5ms��ֹͣ
#define SERVO_MAX_PULSE 2500    // 2.5ms�������ת

// �ٶȰٷֱ�ת����
static uint16_t servo_speed_to_pulse(int speed)
{
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    // �ٶ�Ϊ0ʱΪֹͣ����
    if (speed == 0) return SERVO_STOP_PULSE;
    // ��Ϊ��ת����Ϊ��ת
    if (speed > 0)
        return SERVO_STOP_PULSE + (SERVO_MAX_PULSE - SERVO_STOP_PULSE) * speed / 100;
    else
        return SERVO_STOP_PULSE + (SERVO_MIN_PULSE - SERVO_STOP_PULSE) * (-speed) / 100;
}

volatile int servo_round_count = 10;  // Ҫִ�е�Ȧ�������ⲿ�������ã�
volatile int servo_speed = 50;        // ��ǰ�ٶȣ�-100~100��

// ��������߳�
static void servo_thread_entry(void *parameter)
{
    rt_kprintf("��������߳�����...\n");

    int round_done = 0;
    int last_speed = 0;
    int tick_per_round = 50; // ����ÿȦ1s��20msһ�Σ�50��ΪһȦ�������ʵ�ʶ��������
    int tick_count = 0;

    pwm_tim8_init(SERVO_PERIOD, servo_speed_to_pulse(0));  // ��ʼ��Ϊֹͣ
    TIM_SetCompare1(TIM8, servo_speed_to_pulse(0));

    while (1)
    {
        if (servo_round_count > 0 && servo_speed != 0)
        {
            TIM_SetCompare1(TIM8, servo_speed_to_pulse(servo_speed));
            tick_count++;
            if (tick_count >= tick_per_round)
            {
                tick_count = 0;
                round_done++;
                rt_kprintf("�����Ȧ��: %d/%d\n", round_done, servo_round_count);
            }
            if (round_done >= servo_round_count)
            {
                servo_round_count = 0;
                round_done = 0;
                tick_count = 0;
                servo_speed = 0;
                TIM_SetCompare1(TIM8, servo_speed_to_pulse(0)); // ֹͣ
                rt_kprintf("������ɣ����ֹͣ��\n");
            }
        }
        else
        {
            // ���ٶȱ仯����������PWM
            if (last_speed != servo_speed)
            {
                TIM_SetCompare1(TIM8, servo_speed_to_pulse(servo_speed));
                last_speed = servo_speed;
            }
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
    pwm_tim8_init(SERVO_PERIOD, servo_speed_to_pulse(g_servo_param.speed));
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
    {
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("����̴߳���ʧ�ܣ�\n");
    }
}

// ����̨������ö���ٶ�
static void servo_speed_cmd(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("�÷�: servo_speed �ٶ�(-100~100)\n");
        return;
    }
    int speed = atoi(argv[1]);
    if (speed < -100 || speed > 100)
    {
        rt_kprintf("�ٶȷ�Χ-100~100\n");
        return;
    }
    servo_speed = speed;
    g_servo_param.speed = speed;
    rt_kprintf("���ö���ٶ�: %d\n", speed);
}
#include <finsh.h>
MSH_CMD_EXPORT(servo_speed_cmd, ���ö���ٶ� -100~100);

// ����̨������ö����תȦ��
static void set_round_cmd(int argc, char **argv)
{
    if (argc == 2)
        servo_round_count = atoi(argv[1]);
    rt_kprintf("���ö����תȦ��: %d\n", servo_round_count);
}
MSH_CMD_EXPORT(set_round_cmd, ���ö����תȦ��);

