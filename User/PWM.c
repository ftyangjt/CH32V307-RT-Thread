#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>

// 连续旋转舵机参数结构体
typedef struct
{
    int speed; // 舵机速度（-100~100，0为停止）
} servo_param_t;

servo_param_t g_servo_param = {0}; // 默认停止

// PWM参数
#define SERVO_PERIOD 20000      // 20ms周期，单位us
#define SERVO_MIN_PULSE 500     // 0.5ms，最大反转
#define SERVO_STOP_PULSE 1500   // 1.5ms，停止
#define SERVO_MAX_PULSE 2500    // 2.5ms，最大正转

// 速度百分比转脉宽
static uint16_t servo_speed_to_pulse(int speed)
{
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    // 速度为0时为停止脉宽
    if (speed == 0) return SERVO_STOP_PULSE;
    // 负为反转，正为正转
    if (speed > 0)
        return SERVO_STOP_PULSE + (SERVO_MAX_PULSE - SERVO_STOP_PULSE) * speed / 100;
    else
        return SERVO_STOP_PULSE + (SERVO_MIN_PULSE - SERVO_STOP_PULSE) * (-speed) / 100;
}

volatile int servo_round_count = 10;  // 要执行的圈数（由外部触发设置）
volatile int servo_speed = 50;        // 当前速度（-100~100）

// 舵机控制线程
static void servo_thread_entry(void *parameter)
{
    rt_kprintf("舵机控制线程启动...\n");

    int round_done = 0;
    int last_speed = 0;
    int tick_per_round = 50; // 假设每圈1s，20ms一次，50次为一圈（需根据实际舵机调整）
    int tick_count = 0;

    pwm_tim8_init(SERVO_PERIOD, servo_speed_to_pulse(0));  // 初始化为停止
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
                rt_kprintf("已完成圈数: %d/%d\n", round_done, servo_round_count);
            }
            if (round_done >= servo_round_count)
            {
                servo_round_count = 0;
                round_done = 0;
                tick_count = 0;
                servo_speed = 0;
                TIM_SetCompare1(TIM8, servo_speed_to_pulse(0)); // 停止
                rt_kprintf("动作完成，舵机停止。\n");
            }
        }
        else
        {
            // 若速度变化，立即更新PWM
            if (last_speed != servo_speed)
            {
                TIM_SetCompare1(TIM8, servo_speed_to_pulse(servo_speed));
                last_speed = servo_speed;
            }
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
    pwm_tim8_init(SERVO_PERIOD, servo_speed_to_pulse(g_servo_param.speed));
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
    {
        rt_thread_startup(tid);
    }
    else
    {
        rt_kprintf("舵机线程创建失败！\n");
    }
}

// 控制台命令：设置舵机速度
static void servo_speed_cmd(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("用法: servo_speed 速度(-100~100)\n");
        return;
    }
    int speed = atoi(argv[1]);
    if (speed < -100 || speed > 100)
    {
        rt_kprintf("速度范围-100~100\n");
        return;
    }
    servo_speed = speed;
    g_servo_param.speed = speed;
    rt_kprintf("设置舵机速度: %d\n", speed);
}
#include <finsh.h>
MSH_CMD_EXPORT(servo_speed_cmd, 设置舵机速度 -100~100);

// 控制台命令：设置舵机旋转圈数
static void set_round_cmd(int argc, char **argv)
{
    if (argc == 2)
        servo_round_count = atoi(argv[1]);
    rt_kprintf("设置舵机旋转圈数: %d\n", servo_round_count);
}
MSH_CMD_EXPORT(set_round_cmd, 设置舵机旋转圈数);

