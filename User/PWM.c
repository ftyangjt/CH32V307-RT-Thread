#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

// 舵机参数结构体，为可能添加的功能预留
typedef struct
{
    int angle; // 舵机角度（0~180）
} servo_param_t;

servo_param_t g_servo_param = {100}; // 默认90度

// PWM参数
#define SERVO_PERIOD 1000 * 20 // 20ms周期，单位us
#define SERVO_MIN_PULSE 100 * 5   // 0度对应脉宽0.5ms
#define SERVO_MAX_PULSE 100 * 25  // 180度对应脉宽2.5ms

// 角度转占空比脉宽
static uint16_t servo_angle_to_pulse(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    return (uint16_t)(SERVO_MIN_PULSE + (SERVO_MAX_PULSE - SERVO_MIN_PULSE) * (angle / 180.0f));
}

// 舵机控制线程
static void servo_thread_entry(void *parameter)
{
    rt_kprintf("测试舵机2");
    int last_angle = -1;
    uint16_t period = SERVO_PERIOD;
    pwm_tim8_init(period, servo_angle_to_pulse(g_servo_param.angle));  // 初始化一次

    while (1)
    {
        
        if (g_servo_param.angle != last_angle)
        {
            uint16_t pulse = servo_angle_to_pulse(g_servo_param.angle);
            TIM_SetCompare1(TIM8, pulse);  // ? 只更新占空比，不重新 init
            last_angle = g_servo_param.angle;
            rt_kprintf("舵机角度: %d, 脉宽: %d us\n", g_servo_param.angle, pulse);
        }

        rt_thread_mdelay(20);
    }
}


// PWM初始化
void pwm_module_init(void)
{
    SystemCoreClockUpdate(); // 更新系统核心时钟
    rt_kprintf("测试舵机1");
    pwm_gpio_init();
    pwm_tim8_init(SERVO_PERIOD, servo_angle_to_pulse(g_servo_param.angle));
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
    {
        // rt_kprintf("舵机线程创建成功\n");
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("舵机线程创建失败！\n");
    }
}

// 控制台命令：设置舵机角度
static void servo_angle_cmd(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("用法: servo_angle 角度(0~180)\n");
        return;
    }
    int angle = atoi(argv[1]);
    if (angle < 0 || angle > 180)
    {
        rt_kprintf("角度范围0~180\n");
        return;
    }
    g_servo_param.angle = angle;
    rt_kprintf("设置舵机角度: %d\n", angle);
}
#include <finsh.h>
MSH_CMD_EXPORT(servo_angle_cmd, 设置舵机角度 0~180);

