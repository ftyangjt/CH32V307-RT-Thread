#include "ch32v30x.h"
//pwm驱动！
//笔记在2.md


//GPIO的设置
void pwm_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 打开 GPIOC 和 TIM8 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM8, ENABLE);

    // 配置 PC6 为复用推挽输出（TIM8_CH1）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
}

void pwm_tim8_init(uint16_t period, uint16_t pulse)//周期，脉冲
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    //一，配置TIM
    // 定时器时钟 = SystemCoreClock / 96
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1;               //自动重装值，就是周期
    TIM_TimeBaseStructure.TIM_Prescaler = 96 - 1;                    //72 MHz / (71 + 1) = 1 MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数
    TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

    //二，配置 PWM 输出
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//模式，1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//允许输出
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = pulse;    //占空比
    TIM_OC1Init(TIM8, &TIM_OCInitStructure); //OC1：在1通道应用设置
    TIM_OC1PreloadConfig(TIM8, TIM_OCPreload_Enable);

    // 启动定时器
    TIM_ARRPreloadConfig(TIM8, ENABLE);
    TIM_CtrlPWMOutputs(TIM8, ENABLE);  //高级定时器需要手动使能主输出
    TIM_Cmd(TIM8, ENABLE); //启动定时器
}






// 新增：TIM10的GPIO初始化（PB8）
void pwm_gpio_init_10(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 打开 GPIOB 和 TIM10 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_TIM10, ENABLE);

    // 配置 PB8 为复用推挽输出（TIM10_CH1）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// 新增：TIM10的PWM初始化
void pwm_tim10_init(uint16_t period, uint16_t pulse)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 配置TIM10
    TIM_TimeBaseStructure.TIM_Period = period - 1;               //自动重装值
    TIM_TimeBaseStructure.TIM_Prescaler = 96 - 1;                //与TIM8一致
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM10, &TIM_TimeBaseStructure);

    // 配置 PWM 输出
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = pulse;
    TIM_OC1Init(TIM10, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM10, TIM_OCPreload_Enable);

    // 启动定时器
    TIM_ARRPreloadConfig(TIM10, ENABLE);
    TIM_CtrlPWMOutputs(TIM10, ENABLE);  // 高级定时器需要手动使能主输出
    TIM_Cmd(TIM10, ENABLE);
}
