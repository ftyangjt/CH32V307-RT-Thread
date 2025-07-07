#include "drv_pwm.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include "Cardinal.h"

// 舵机参数结构体：速度（-100~100），持续时间（秒）
typedef struct
{
    int speed;
    int duration_sec;
} servo_param_t;

servo_param_t g_servo_param = {10, 3600}; // 测试：不要停下来


// 查FS90R参数，得到：
// FS90R是360度连续旋转舵机，通过占空比控制旋转速度和方向
#define SERVO_PERIOD      20000  // 20ms周期，单位us
#define SERVO_MIN_PULSE    900   // 0.9ms，最大逆时针旋转
#define SERVO_STOP_PULSE  1500   // 1.5ms，停止
#define SERVO_MAX_PULSE   2100   // 2.1ms，最大顺时针旋转

// 死区与最小速度定义
#define SERVO_DEADZONE      10    // 死区阈值（FS90R死区较小）
#define SERVO_MIN_RUN_SPEED 15    // 最小有效速度

//根据参数，将速度转换为高电平时间的函数（带死区与最小速度限制）
static uint16_t servo_speed_to_pulse(int speed)
{
    // 死区处理
    if (speed > -SERVO_DEADZONE && speed < SERVO_DEADZONE)
        return SERVO_STOP_PULSE;
    // 最小速度限制
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

// 舵机控制线程入口
static void servo_thread_entry(void *parameter)
{
    struct rt_semaphore *servo_sem = g_cardinal_servo_sem;
    extern struct rt_semaphore *g_cardinal_servo_done_sem;

    while (1)
    {
        // 等待主控线程释放信号量
        rt_sem_take(servo_sem, RT_WAITING_FOREVER);
        // 启动PWM输出

        if (g_servo_param.speed != 0 && g_servo_param.duration_sec > 0)
        {
            int speed = g_servo_param.speed;
            int duration = g_servo_param.duration_sec;
            uint16_t pulse = servo_speed_to_pulse(speed);
            rt_kprintf("舵机启动，速度: %d，持续: %d 秒，脉宽: %d us\n", speed, duration, pulse);
            speed = g_servo_param.speed;
            pulse = servo_speed_to_pulse(speed);
            TIM_SetCompare1(TIM10, pulse);

            for (int i = 0; i < 100 ; i++)
            {
                rt_thread_mdelay(20);
            } //等待TIM调正好PWM！！！！！！！！！！！！！

            TIM_CCxCmd(TIM10, TIM_Channel_1, ENABLE);

            for (int i = 0; i < duration * 50; i++)
            {
                rt_thread_mdelay(20);
            }

            //关闭PWM输出，防止舵机持续振动
            TIM_CCxCmd(TIM10, TIM_Channel_1, DISABLE);
            g_servo_param.speed = 0;
            g_servo_param.duration_sec = 0;
            rt_kprintf("舵机动作完成，已停止。\n");
        }

        if (g_cardinal_servo_done_sem)
            rt_sem_release(g_cardinal_servo_done_sem);
    }
}

//PWM初始化
void pwm_module_init(void)
{
    SystemCoreClockUpdate();
    rt_kprintf("PWM舵机模块初始化（FS90R）\n");
    pwm_gpio_init_10();
    pwm_tim10_init(SERVO_PERIOD, servo_speed_to_pulse(0));
    // 初始化时不输出PWM，关闭通道
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    TIM_CCxCmd(TIM10, TIM_Channel_1, DISABLE);
    TIM_Cmd(TIM10, ENABLE);
    rt_thread_t tid = rt_thread_create("servo", servo_thread_entry, RT_NULL, 1024, 8, 10);
    if (tid)
        rt_thread_startup(tid);
    else
        rt_kprintf("舵机线程创建失败！\n");
}

//控制台命令：设置舵机速度和旋转秒数
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

//控制台命令：立即停止舵机
static void servo_stop_cmd(int argc, char **argv)
{
    g_servo_param.speed = 0;
    g_servo_param.duration_sec = 0;
    TIM_SetCompare1(TIM10, servo_speed_to_pulse(0));
    // 立即关闭PWM输出
    TIM_CCxCmd(TIM10, TIM_Channel_1, DISABLE);
    rt_kprintf("舵机已停止\n");
}
MSH_CMD_EXPORT(servo_stop_cmd, 立即停止舵机);

