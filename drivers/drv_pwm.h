#ifndef __DRV_PWM_H__
#define __DRV_PWM_H__

#include "ch32v30x.h"

// 初始化PWM相关GPIO
void pwm_gpio_init(void);

// 初始化TIM8为PWM输出，period为周期（us），pulse为高电平脉宽（us）
void pwm_tim8_init(uint16_t period, uint16_t pulse);

#endif // __DRV_PWM_H__
