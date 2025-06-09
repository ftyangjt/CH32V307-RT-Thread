#ifndef __PWM_H__
#define __PWM_H__

#include <rtthread.h>
#include <rtdevice.h>

// 舵机参数结构体声明
typedef struct
{
    float angle; // 舵机角度（0~180）
} servo_param_t;

// 全局舵机参数变量声明
extern servo_param_t g_servo_param;

// PWM初始化函数声明
void pwm_module_init(void);

// 控制台命令声明（可选）
void servo_angle_cmd(int argc, char **argv);

#endif // __PWM_H__
