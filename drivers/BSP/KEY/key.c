/**
 ****************************************************************************************************
 * @file        key.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       �������� ��������
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� CH32V307������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20230720
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "./BSP/KEY/key.h"
#include "./SYSTEM/delay/delay.h"
#include "rtthread.h"


/**
 * @brief       ������ʼ������
 * @param       ��
 * @retval      ��
 */
void key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;                          /* GPIO���ò����洢���� */
    KEY0_GPIO_CLK_ENABLE();                                     /* KEY0ʱ��ʹ�� */
    KEY1_GPIO_CLK_ENABLE();                                     /* KEY1ʱ��ʹ�� */
    WKUP_GPIO_CLK_ENABLE();                                     /* WKUPʱ��ʹ�� */

    gpio_init_struct.GPIO_Pin = KEY0_GPIO_PIN;                  /* KEY0���� */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;                 /* �������� */
    GPIO_Init(KEY0_GPIO_PORT, &gpio_init_struct);               /* KEY0����ģʽ����,�������� */

    gpio_init_struct.GPIO_Pin = KEY1_GPIO_PIN;                  /* KEY1���� */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;                 /* �������� */
    GPIO_Init(KEY1_GPIO_PORT, &gpio_init_struct);               /* KEY1����ģʽ����,�������� */

    gpio_init_struct.GPIO_Pin = WKUP_GPIO_PIN;                  /* WKUP���� */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPD;                 /* �������� */
    GPIO_Init(WKUP_GPIO_PORT, &gpio_init_struct);               /* WKUP����ģʽ����,�������� */

}

/**
 * @brief       ����ɨ�躯��
 * @note        �ú�������Ӧ���ȼ�(ͬʱ���¶������): WK_UP > KEY1 > KEY0!!
 * @param       mode:0 / 1, ���庬������:
 *   @arg       0,  ��֧��������(���������²���ʱ, ֻ�е�һ�ε��û᷵�ؼ�ֵ,
 *                  �����ɿ��Ժ�, �ٴΰ��²Ż᷵��������ֵ)
 *   @arg       1,  ֧��������(���������²���ʱ, ÿ�ε��øú������᷵�ؼ�ֵ)
 * @retval      ��ֵ, ��������:
 *              KEY0_PRES, 1, KEY0����
 *              KEY1_PRES, 2, KEY1����
 *              WKUP_PRES, 3, WKUP����
 */
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;  /* �������ɿ���־ */
    uint8_t keyval = 0;

    if (mode) key_up = 1;       /* ֧������ */

    if (key_up && (KEY0 == 0 || KEY1 == 0 || WK_UP == 1))  /* �����ɿ���־Ϊ1, ��������һ������������ */
    {
        rt_thread_mdelay(10);           /* ȥ���� */
        key_up = 0;

        if (KEY0 == 0)  keyval = KEY0_PRES;

        if (KEY1 == 0)  keyval = KEY1_PRES;

        if (WK_UP == 1) keyval = WKUP_PRES;
    }
    else if (KEY0 == 1 && KEY1 == 1 && WK_UP == 0) /* û���κΰ�������, ��ǰ����ɿ� */
    {
        key_up = 1;
    }

    return keyval;              /* ���ؼ�ֵ */
}




















