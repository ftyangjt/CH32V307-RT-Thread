#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include "Cardinal.h"

// ��������ṹ�壺�ٶȣ�-100~100��������ʱ�䣨�룩
typedef struct
{
    int speed;
    int duration_sec;
} servo_param_t;

servo_param_t g_servo_param = {10, 3600}; // ���ԣ���Ҫͣ����


// ��FS90R�������õ���
// FS90R��360��������ת�����ͨ��ռ�ձȿ�����ת�ٶȺͷ���
#define SERVO_PERIOD      20000  // 20ms���ڣ���λus
#define SERVO_MIN_PULSE    900   // 0.9ms�������ʱ����ת
#define SERVO_STOP_PULSE  1500   // 1.5ms��ֹͣ
#define SERVO_MAX_PULSE   2100   // 2.1ms�����˳ʱ����ת

// ��������С�ٶȶ���
#define SERVO_DEADZONE      10    // ������ֵ��FS90R������С��
#define SERVO_MIN_RUN_SPEED 15    // ��С��Ч�ٶ�

//���ݲ��������ٶ�ת��Ϊ�ߵ�ƽʱ��ĺ���������������С�ٶ����ƣ�
static uint16_t servo_speed_to_pulse(int speed)
{
    // ��������
    if (speed > -SERVO_DEADZONE && speed < SERVO_DEADZONE)
        return SERVO_STOP_PULSE;
    // ��С�ٶ�����
    if (speed > 0 && speed < SERVO_MIN_RUN_SPEED)
        speed = SERVO_MIN_RUN_SPEED;
    if (speed < 0 && speed > -SERVO_MIN_RUN_SPEED)
        speed = -SERVO_MIN_RUN_SPEED;
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    if (speed == 0) return SERVO_STOP_PULSE;
    if (speed > 0)
        return SERVO_STOP_PULSE + (SERVO_MAX_PULSE - SERVO_STOP_PULSE) * speed / 100;
    else
        return SERVO_STOP_PULSE - (SERVO_STOP_PULSE - SERVO_MIN_PULSE) * (-speed) / 100;
}

// ��������߳����
static void servo_thread_entry(void *parameter)
{
    struct rt_semaphore *servo_sem = g_cardinal_servo_sem;
    extern struct rt_semaphore *g_cardinal_servo_done_sem;

    while (1)
    {
        // �ȴ������߳��ͷ��ź���
        rt_sem_take(servo_sem, RT_WAITING_FOREVER);
        // ����PWM���

        if (g_servo_param.speed != 0 && g_servo_param.duration_sec > 0)
        {
            int speed = g_servo_param.speed;
            int duration = g_servo_param.duration_sec;
            uint16_t pulse = servo_speed_to_pulse(speed);
            rt_kprintf("����������ٶ�: %d������: %d �룬����: %d us\n", speed, duration, pulse);
            speed = g_servo_param.speed;
            pulse = servo_speed_to_pulse(speed);
            TIM_SetCompare1(TIM10, pulse);

            for (int i = 0; i < 100 ; i++)
            {
                rt_thread_mdelay(20);
            } //�ȴ�TIM������PWM��������������������������

            TIM_CCxCmd(TIM10, TIM_Channel_1, ENABLE);

            for (int i = 0; i < duration * 50; i++)
            {
                rt_thread_mdelay(20);
            }

            //�ر�PWM�������ֹ���������
            TIM_CCxCmd(TIM10, TIM_Channel_1, DISABLE);
            g_servo_param.speed = 0;
            g_servo_param.duration_sec = 0;
            rt_kprintf("���������ɣ���ֹͣ��\n");
        }

        if (g_cardinal_servo_done_sem)
            rt_sem_release(g_cardinal_servo_done_sem);
    }
}

//PWM��ʼ��
void pwm_module_init(void)
{
    SystemCoreClockUpdate();
    rt_kprintf("PWM���ģ���ʼ����FS90R��\n");
    pwm_gpio_init_10();
    pwm_tim10_init(SERVO_PERIOD, servo_speed_to_pulse(0));
    // ��ʼ��ʱ�����PWM���ر�ͨ��
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    TIM_CCxCmd(TIM10, TIM_Channel_1, DISABLE);
    TIM_Cmd(TIM10, ENABLE);
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
        rt_thread_startup(tid);
    else
        rt_kprintf("����̴߳���ʧ�ܣ�\n");
}

//����̨������ö���ٶȺ���ת����
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

//����̨�������ֹͣ���
static void servo_stop_cmd(int argc, char **argv)
{
    g_servo_param.speed = 0;
    g_servo_param.duration_sec = 0;
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    // �����ر�PWM���
    TIM_CCxCmd(TIM10, TIM_Channel_1, DISABLE);
    rt_kprintf("�����ֹͣ\n");
}
MSH_CMD_EXPORT(servo_stop_cmd, ����ֹͣ���);

