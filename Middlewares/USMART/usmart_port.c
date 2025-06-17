/**
 ****************************************************************************************************
 * @file        usmart_port.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V3.5
 * @date        2020-12-20
 * @brief       USMART ��ֲ�ļ�
 *
 *              ͨ���޸ĸ��ļ�,���Է���Ľ�USMART��ֲ����������
 *              ��:USMART_ENTIMX_SCAN == 0ʱ,����Ҫʵ��: usmart_get_input_string����.
 *              ��:USMART_ENTIMX_SCAN == 1ʱ,��Ҫ��ʵ��4������:
 *              usmart_timx_reset_time
 *              usmart_timx_get_time
 *              usmart_timx_init
 *              USMART_TIMX_IRQHandler
 *
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 *
 * V3.4֮ǰ�汾��ϸ�޸�˵����USMART�ļ����µ�:readme.txt
 *
 * V3.4 20200324
 * 1, ����usmart_port.c��usmart_port.h,���ڹ���USMART����ֲ,�����޸�
 * 2, �޸ı���������ʽΪ: uint8_t, uint16_t, uint32_t
 * 3, �޸�usmart_reset_runtimeΪusmart_timx_reset_time
 * 4, �޸�usmart_get_runtimeΪusmart_timx_get_time
 * 5, �޸�usmart_scan����ʵ�ַ�ʽ,�ĳ���usmart_get_input_string��ȡ������
 * 6, �޸�printf����Ϊprintf�궨��
 * 7, �޸Ķ�ʱɨ����غ���,���ú궨�巽ʽ,������ֲ
 *
 * V3.5 20201220
 * 1���޸Ĳ��ִ���
 *
 ****************************************************************************************************
 */

#include "./USMART/usmart.h"
#include "./USMART/usmart_port.h"

void USMART_TIMX_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/**
 * @brief       ��ȡ����������(�ַ���)
 *   @note      USMARTͨ�������ú������ص��ַ����Ի�ȡ����������������Ϣ
 * @param       ��
 * @retval
 *   @arg       0,  û�н��յ�����
 *   @arg       ����,�������׵�ַ(������0)
 */
char *usmart_get_input_string(void)
{
    uint8_t len;
    char *pbuf = 0;

    if (g_usart_rx_sta & 0x8000)        /* ���ڽ�����ɣ� */
    {
        len = g_usart_rx_sta & 0x3fff;  /* �õ��˴ν��յ������ݳ��� */
        g_usart_rx_buf[len] = '\0';     /* ��ĩβ���������. */
        pbuf = (char*)g_usart_rx_buf;
        g_usart_rx_sta = 0;             /* ������һ�ν��� */
    }

    return pbuf;
}

/* ���ʹ���˶�ʱ��ɨ��, ����Ҫ�������º��� */
#if USMART_ENTIMX_SCAN == 1

/**
 * ��ֲע��:��������ch32Ϊ��,���Ҫ��ֲ������mcu,������Ӧ�޸�.
 * usmart_reset_runtime,�����������ʱ��,��ͬ��ʱ���ļ����Ĵ����Լ���־λһ������.��������װ��ֵΪ���,������޶ȵ��ӳ���ʱʱ��.
 * usmart_get_runtime,��ȡ��������ʱ��,ͨ����ȡCNTֵ��ȡ,����usmart��ͨ���жϵ��õĺ���,���Զ�ʱ���жϲ�����Ч,��ʱ����޶�
 * ֻ��ͳ��2��CNT��ֵ,Ҳ���������+���һ��,���������2��,û������,���������ʱ,������:2*������CNT*0.1ms
 * ������:USMART_TIMX_IRQHandler��usmart_timx_init,��Ҫ����MCU�ص������޸�.ȷ������������Ƶ��Ϊ:10Khz����.����,��ʱ����Ҫ�����Զ���װ�ع���!!
 */

/**
 * @brief       ��λruntime
 *   @note      ��Ҫ��������ֲ����MCU�Ķ�ʱ�����������޸�
 * @param       ��
 * @retval      ��
 */
void usmart_timx_reset_time(void)
{
    TIM_ClearFlag(USMART_TIMX, TIM_FLAG_Update); /* ����жϱ�־λ */
    TIM_SetAutoreload(USMART_TIMX, 0XFFFF);      /* ����װ��ֵ���õ���� */
    TIM_SetCounter(USMART_TIMX, 0);              /* ��ն�ʱ����CNT */
    usmart_dev.runtime = 0;
}

/**
 * @brief       ���runtimeʱ��
 *   @note      ��Ҫ��������ֲ����MCU�Ķ�ʱ�����������޸�
 * @param       ��
 * @retval      ִ��ʱ��,��λ:0.1ms,�����ʱʱ��Ϊ��ʱ��CNTֵ��2��*0.1ms
 */
uint32_t usmart_timx_get_time(void)
{
    if (TIM_GetFlagStatus(USMART_TIMX, TIM_FLAG_Update) == SET)  /* �������ڼ�,�����˶�ʱ����� */
    {
        usmart_dev.runtime += 0XFFFF;
    }
    usmart_dev.runtime += TIM_GetCounter(USMART_TIMX);
    return usmart_dev.runtime;                                   /* ���ؼ���ֵ */
}

/**
 * @brief       ��ʱ����ʼ������
 * @param       arr:�Զ���װ��ֵ
 *              psc:��ʱ����Ƶϵ��
 * @retval      ��
 */ 
void usmart_timx_init(uint16_t arr, uint16_t psc)
{ 
    TIM_TimeBaseInitTypeDef  timx_usmart_handle;
    NVIC_InitTypeDef nvic_init_struct;

    USMART_TIMX_CLK_ENABLE();

    timx_usmart_handle.TIM_Prescaler = psc;                             /* ��Ƶϵ�� */
    timx_usmart_handle.TIM_CounterMode = TIM_CounterMode_Up;            /* ���ϼ����� */
    timx_usmart_handle.TIM_Period = arr;                                /* �Զ�װ��ֵ */
    timx_usmart_handle.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(USMART_TIMX,&timx_usmart_handle);

    TIM_ITConfig(USMART_TIMX,TIM_IT_Update,ENABLE );                    /* ʹ��ָ����TIMx�ж�,��������ж� */
    TIM_Cmd(USMART_TIMX, ENABLE);                                       /* ʹ��TIMx */

    nvic_init_struct.NVIC_IRQChannel = USMART_TIMX_IRQn;                /* ѡ��TIMx�ж� */
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 3;             /* ��ռ���ȼ�3 */
    nvic_init_struct.NVIC_IRQChannelSubPriority = 3;                    /* ��Ӧ���ȼ�3 */
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;                       /* IRQͨ����ʹ�� */
    NVIC_Init(&nvic_init_struct);                                       /* ��ʼ��NVIC */
}

/**
 * @brief       USMART��ʱ���жϷ�����
 * @param       ��
 * @retval      ��
 */
void USMART_TIMX_IRQHandler(void)
{
    if(TIM_GetITStatus(USMART_TIMX, TIM_IT_Update) == SET)              /* ����ж� */
    {
        usmart_dev.scan();                                              /* ִ��usmartɨ�� */
        TIM_SetCounter(USMART_TIMX, 0);                               /* ��ն�ʱ����CNT */
        TIM_SetAutoreload(USMART_TIMX, 100);                            /* �ָ�ԭ�������� */
    }
    
    TIM_ClearITPendingBit(USMART_TIMX, TIM_IT_Update);                  /* ����жϱ�־λ */
}

#endif


