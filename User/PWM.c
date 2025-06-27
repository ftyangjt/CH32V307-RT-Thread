#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

// 舵机参数结构体：速度（-100~100），持续时间（秒）
typedef struct
{
    int speed;
    int duration_sec;
} servo_param_t;

servo_param_t g_servo_param = {10, 999999}; // 默认停止

#define SERVO_PERIOD      20000  // 20ms周期，单位us
#define SERVO_MIN_PULSE    500   // 0.5ms，最大反转
#define SERVO_STOP_PULSE  1500   // 1.5ms，停止
#define SERVO_MAX_PULSE   2500   // 2.5ms，最大正转

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

// 舵机控制线程
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
            rt_kprintf("舵机启动，速度: %d，持续: %d 秒\n", g_servo_param.speed, g_servo_param.duration_sec);
            TIM_SetCompare1(TIM10, servo_speed_to_pulse(g_servo_param.speed));
            last_speed = g_servo_param.speed;
            for (int i = 0; i < g_servo_param.duration_sec * 50; i++) // 20ms*50=1s
            {
                rt_thread_mdelay(20);
                // 若期间速度被外部命令更改，立即响应
                if (g_servo_param.speed != last_speed)
                {
                    TIM_SetCompare1(TIM10, servo_speed_to_pulse(g_servo_param.speed));
                    last_speed = g_servo_param.speed;
                }
            }
            // 停止
            TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
            g_servo_param.speed = 0;
            g_servo_param.duration_sec = 0;
            rt_kprintf("舵机动作完成，已停止。\n");
        }
        else
        {
            // 若速度变化，立即更新PWM
            if (last_speed != g_servo_param.speed)
            {
                TIM_SetCompare1(TIM10, servo_speed_to_pulse(g_servo_param.speed));
                last_speed = g_servo_param.speed;
            }
            rt_thread_mdelay(20);
        }
    }
}

// PWM初始化
void pwm_module_init(void)
{
    SystemCoreClockUpdate();
    rt_kprintf("PWM舵机模块初始化\n");
    pwm_gpio_init_10();
    pwm_tim10_init(SERVO_PERIOD, servo_speed_to_pulse(0));
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    TIM_Cmd(TIM10, ENABLE);
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
        rt_thread_startup(tid);
    else
        rt_kprintf("舵机线程创建失败！\n");
}

// 控制台命令：设置舵机速度和旋转秒数
static void servo_run_cmd(int argc, char **argv)
{
    if (argc != 3)
    {
        rt_kprintf("用法: servo_run 速度(-100~100) 秒数\n");
        return;
    }
    int speed = atoi(argv[1]);
    int duration = atoi(argv[2]);
    if (speed < -100 || speed > 100)
    {
        rt_kprintf("速度范围-100~100\n");
        return;
    }
    if (duration <= 0)
    {
        rt_kprintf("持续时间需大于0秒\n");
        return;
    }
    g_servo_param.speed = speed;
    g_servo_param.duration_sec = duration;
    rt_kprintf("设置舵机速度: %d, 持续: %d 秒\n", speed, duration);
}
#include <finsh.h>
MSH_CMD_EXPORT(servo_run_cmd, 设置舵机速度和旋转秒数: servo_run 速度 秒数);

// 控制台命令：立即停止舵机
static void servo_stop_cmd(int argc, char **argv)
{
    g_servo_param.speed = 0;
    g_servo_param.duration_sec = 0;
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    rt_kprintf("舵机已停止\n");
}
MSH_CMD_EXPORT(servo_stop_cmd, 立即停止舵机);

