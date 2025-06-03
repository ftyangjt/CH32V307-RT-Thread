/********************************** (C) COPYRIGHT *******************************
* File Name          : ws2812.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/05/27
* Description        : WS2812 RGB LED驱动
*********************************************************************************/

#include "ws2812.h"
#include <math.h>  // 用于数学计算

// 全局变量
static uint8_t led_rgb_data[WS2812_LED_NUM * 3];
static uint16_t pwm_data[24 * WS2812_LED_NUM + 50];  // 24位/LED + 50位复位码

/* TIM2 PWM初始化 */
static void tim2_pwm_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 使能GPIO和TIM2时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 配置PA0为TIM2_CH1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 计算TIM2分频系数和重载值
    uint32_t timer_clock = SystemCoreClock;  // 系统时钟
    uint16_t prescaler = 1;  // 预分频值
    uint16_t period = timer_clock / (prescaler * WS2812_PWM_FREQ) - 1;  // 计算周期
    
    // 配置TIM2基本参数
    TIM_TimeBaseInitStructure.TIM_Period = period;  
    TIM_TimeBaseInitStructure.TIM_Prescaler = prescaler - 1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    
    // 配置TIM2通道1为PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;  // 初始占空比为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    
    // 使能TIM2_CH1预装载
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    
    // 使能TIM2
    TIM_Cmd(TIM2, ENABLE);
    
    rt_kprintf("TIM2 PWM初始化完成，频率=%dHz, 周期=%d\n", WS2812_PWM_FREQ, period);
}

/* DMA初始化 */
static void dma_init_for_tim2(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // 使能DMA1时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 配置DMA1通道5（对应TIM2_CH1）
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&TIM2->CH1CVR;  // 目标为TIM2_CH1的比较寄存器
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pwm_data;  // 源为PWM数据数组
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  // 方向：内存到外设
    DMA_InitStructure.DMA_BufferSize = 24 * WS2812_LED_NUM + 50;  // 传输数量
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // 外设地址不增加
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  // 内存地址递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;  // 外设数据宽度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;  // 内存数据宽度为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  // 非循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;  // 高优先级
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  // 非内存到内存模式
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    // 使能TIM2的DMA请求
    TIM_DMACmd(TIM2, TIM_DMA_CC1, ENABLE);
    
    rt_kprintf("DMA初始化完成，用于WS2812控制\n");
}

/* 将RGB数据转换为PWM占空比 */
static void rgb_to_pwm(uint8_t *rgb_data, uint16_t *pwm_data)
{
    uint32_t bit_index = 0;
    
    // 处理每个LED的数据
    for(int i = 0; i < WS2812_LED_NUM; i++) {
        uint8_t g = rgb_data[i * 3 + 1]; // 先发送G
        uint8_t r = rgb_data[i * 3 + 0]; // 再发送R
        uint8_t b = rgb_data[i * 3 + 2]; // 最后发送B
        
        // 处理G字节（8位）
        for(int j = 7; j >= 0; j--) {
            pwm_data[bit_index++] = (g & (1 << j)) ? WS2812_BIT_1_DUTY : WS2812_BIT_0_DUTY;
        }
        
        // 处理R字节（8位）
        for(int j = 7; j >= 0; j--) {
            pwm_data[bit_index++] = (r & (1 << j)) ? WS2812_BIT_1_DUTY : WS2812_BIT_0_DUTY;
        }
        
        // 处理B字节（8位）
        for(int j = 7; j >= 0; j--) {
            pwm_data[bit_index++] = (b & (1 << j)) ? WS2812_BIT_1_DUTY : WS2812_BIT_0_DUTY;
        }
    }
    
    // 添加复位码（50个0值，确保至少50us低电平）
    for(int i = 0; i < 50; i++) {
        pwm_data[bit_index++] = 0;
    }
}

/* HSV转RGB函数 */
void ws2812_hsv_to_rgb(ws2812_hsv_t hsv, uint8_t* r, uint8_t* g, uint8_t* b)
{
    float hh, p, q, t, ff;
    long i;
    float r_f, g_f, b_f;

    if(hsv.s <= 0.0) {
        r_f = g_f = b_f = hsv.v;
    } else {
        hh = hsv.h;
        if(hh >= 360.0) hh = 0.0;
        hh /= 60.0;
        i = (long)hh;
        ff = hh - i;
        p = hsv.v * (1.0 - hsv.s);
        q = hsv.v * (1.0 - (hsv.s * ff));
        t = hsv.v * (1.0 - (hsv.s * (1.0 - ff)));

        switch(i) {
            case 0: r_f = hsv.v; g_f = t; b_f = p; break;
            case 1: r_f = q; g_f = hsv.v; b_f = p; break;
            case 2: r_f = p; g_f = hsv.v; b_f = t; break;
            case 3: r_f = p; g_f = q; b_f = hsv.v; break;
            case 4: r_f = t; g_f = p; b_f = hsv.v; break;
            case 5:
            default: r_f = hsv.v; g_f = p; b_f = q; break;
        }
    }

    *r = (uint8_t)(r_f * 255);
    *g = (uint8_t)(g_f * 255);
    *b = (uint8_t)(b_f * 255);
}

/* 初始化WS2812 */
void ws2812_init(void)
{
    // 初始化PWM和DMA
    tim2_pwm_init();
    dma_init_for_tim2();
    
    // 初始化LED数据，全部设置为熄灭状态
    for(int i = 0; i < WS2812_LED_NUM * 3; i++) {
        led_rgb_data[i] = 0;
    }
    
    // 发送初始化数据
    ws2812_update(led_rgb_data);
}

/* 发送数据到WS2812 */
void ws2812_update(uint8_t *rgb_data)
{
    // 转换RGB数据为PWM占空比
    rgb_to_pwm(rgb_data, pwm_data);
    
    // 配置DMA传输
    DMA_SetCurrDataCounter(DMA1_Channel5, 24 * WS2812_LED_NUM + 50);
    
    // 开始DMA传输
    DMA_Cmd(DMA1_Channel5, ENABLE);
    
    // 等待传输完成
    while(DMA_GetFlagStatus(DMA1_FLAG_TC5) == RESET);
    
    // 清除传输完成标志
    DMA_ClearFlag(DMA1_FLAG_TC5);
    DMA_Cmd(DMA1_Channel5, DISABLE);
    
    // 确保复位码被正确发送（低电平至少50us）
    rt_thread_mdelay(1);
}