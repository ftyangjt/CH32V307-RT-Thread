/**
 ****************************************************************************************************
 * @file        delay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       提供delay_init初始化函数， delay_us和delay_ms等延时函数
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 CH32V307开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com/forum.php
 * 公司网址:www.alientek.com
 * 购买地址:zhengdianyuanzi.tmall.com
 *
 * 修改说明
 * V1.0 20230720
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/delay/delay.h"


static uint16_t g_fac_us = 0;      /* us延时倍乘数 */

/**
 * @brief       初始化延迟函数
 * @param       sysclk: 系统时钟频率,即CPU频率(HCLK),这里以MHz为单位
 * @retval      无
 */
void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk / 8;
}

/**
 * @brief       延时nus
 * @param       nus: 要延时的us数
 * @retval      无
 */
void delay_us(uint32_t nus)
{
    uint32_t temp;

    SysTick->CTLR = (1<<4);                  /* 向下计数 */
    temp = (uint32_t)nus * g_fac_us;         /* 计算比较值 */

    SysTick->CMP = temp;                     /* 设置比较值 */
    SysTick->CTLR |= (1<<5)|(1<<0);          /* 更新比较值，启动计数器 */

    while((SysTick->SR & (1<<0)) != (1<<0)); /* 等待计数溢出 */
    SysTick->SR &= ~(1<<0);                  /* 清除溢出标志 */
}

/**
 * @brief       延时nms
 * @param       nms: 要延时的ms数 (0< nms <= 65535)
 * @retval      无
 */
void delay_ms(uint16_t nms)
{
    uint32_t repeat = nms / 1000;
    uint32_t remain = nms % 1000;

    while (repeat)
    {
        delay_us(1000 * 1000);      /* 利用delay_us实现1000ms延时 */
        repeat--;
    }

    if (remain)
    {
        delay_us(remain * 1000);    /* 利用delay_us,把尾数延时(remain ms)给做了 */
    }
}
