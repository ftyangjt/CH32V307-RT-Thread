/**
 ****************************************************************************************************
 * @file        delay.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       �ṩdelay_init��ʼ�������� delay_us��delay_ms����ʱ����
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� CH32V307������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com/forum.php
 * ��˾��ַ:www.alientek.com
 * �����ַ:zhengdianyuanzi.tmall.com
 *
 * �޸�˵��
 * V1.0 20230720
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/delay/delay.h"


static uint16_t g_fac_us = 0;      /* us��ʱ������ */

/**
 * @brief       ��ʼ���ӳٺ���
 * @param       sysclk: ϵͳʱ��Ƶ��,��CPUƵ��(HCLK),������MHzΪ��λ
 * @retval      ��
 */
void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk / 8;
}

/**
 * @brief       ��ʱnus
 * @param       nus: Ҫ��ʱ��us��
 * @retval      ��
 */
void delay_us(uint32_t nus)
{
    uint32_t temp;

    SysTick->CTLR = (1<<4);                  /* ���¼��� */
    temp = (uint32_t)nus * g_fac_us;         /* ����Ƚ�ֵ */

    SysTick->CMP = temp;                     /* ���ñȽ�ֵ */
    SysTick->CTLR |= (1<<5)|(1<<0);          /* ���±Ƚ�ֵ������������ */

    while((SysTick->SR & (1<<0)) != (1<<0)); /* �ȴ�������� */
    SysTick->SR &= ~(1<<0);                  /* ��������־ */
}

/**
 * @brief       ��ʱnms
 * @param       nms: Ҫ��ʱ��ms�� (0< nms <= 65535)
 * @retval      ��
 */
void delay_ms(uint16_t nms)
{
    uint32_t repeat = nms / 1000;
    uint32_t remain = nms % 1000;

    while (repeat)
    {
        delay_us(1000 * 1000);      /* ����delay_usʵ��1000ms��ʱ */
        repeat--;
    }

    if (remain)
    {
        delay_us(remain * 1000);    /* ����delay_us,��β����ʱ(remain ms)������ */
    }
}
