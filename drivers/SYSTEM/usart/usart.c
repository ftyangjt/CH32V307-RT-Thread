/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       ���ڳ�ʼ������(һ���Ǵ���1)��֧��printf
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

#include "./SYSTEM/usart/usart.h"


#if USART_EN_RX                         /* ���ʹ���˽��� */

void USART1_IRQHandler_WCH(void) __attribute__((interrupt("WCH-Interrupt-fast")));

uint8_t g_usart_rx_buf[USART_REC_LEN];  /* ���ջ���, ���USART_REC_LEN���ֽ� */

/*  ����״̬
 *  bit15��      ������ɱ�־
 *  bit14��      ���յ�0x0d
 *  bit13~0��    ���յ�����Ч�ֽ���Ŀ
*/
uint16_t g_usart_rx_sta = 0;

#endif

/**
 * @brief       ����X��ʼ������
 * @param       ��
 * @retval      ��
 */
void usart_init(uint32_t bound)
{
    GPIO_InitTypeDef  gpio_init_struct;
    USART_InitTypeDef uartx_init_struct;
    NVIC_InitTypeDef  nvic_init_struct;

    USART_TX_GPIO_CLK_ENABLE();                              /* ʹ�ܴ���TX��ʱ�� */
    USART_RX_GPIO_CLK_ENABLE();                              /* ʹ�ܴ���RX��ʱ�� */
    USART_UX_CLK_ENABLE();                                   /* ʹ�ܴ���ʱ�� */

    gpio_init_struct.GPIO_Pin = USART_TX_GPIO_PIN;           /* ���ڷ������ź� */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;            /* ����������� */
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;          /* IO�ٶ�����Ϊ50M */
    GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.GPIO_Pin = USART_RX_GPIO_PIN;           /* ����RX�� ģʽ���� */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;      /* �������� */
    GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);        /* ����RX��,���ó�����ģʽ */

    /* USART��ʼ������*/
    uartx_init_struct.USART_BaudRate = bound;                                       /* ������ */
    uartx_init_struct.USART_WordLength = USART_WordLength_8b;                       /* �ֳ�Ϊ8λ���ݸ�ʽ */
    uartx_init_struct.USART_StopBits = USART_StopBits_1;                            /* һ��ֹͣλ */
    uartx_init_struct.USART_Parity = USART_Parity_No;                               /* ����żУ��λ */
    uartx_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   /* ��Ӳ������ */
    uartx_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                   /* �շ�ģʽ */
    USART_Init(USART_UX,&uartx_init_struct);                                        /* ��ʼ������ */

#if USART_EN_RX
    nvic_init_struct.NVIC_IRQChannel = USART_UX_IRQn;        /* ʹ��USART�ж�ͨ�� */
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority=3 ;   /* ��ռ���ȼ�3 */
    nvic_init_struct.NVIC_IRQChannelSubPriority = 3;         /* ��Ӧ���ȼ�3 */
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;            /* IRQͨ��ʹ��  */
    NVIC_Init(&nvic_init_struct);                            /* ��ʼ��NVIC */

    USART_ITConfig(USART_UX, USART_IT_RXNE, ENABLE);         /* �������ڽ����ж� */
#endif
    USART_Cmd(USART_UX, ENABLE);                             /* ʹ�ܴ��� */
}

#if USART_EN_RX                                              /* ���ʹ���˽��� */
/**
 * @brief       ����X�жϷ�����
 * @param       ��
 * @retval      ��
 */
void USART_UX_IRQHandler(void)
{
    uint8_t Res;

    if(USART_GetITStatus(USART_UX, USART_IT_RXNE) != RESET)  /* ����Ǵ���x */
    {
        USART_ClearITPendingBit(USART_UX,USART_IT_RXNE);     /* ����жϱ�־ */
        Res =USART_ReceiveData(USART_UX);                    /* ��ȡ���յ������� */

        if ((g_usart_rx_sta & 0x8000) == 0)                  /* ����δ��� */
        {
            if (g_usart_rx_sta & 0x4000)                     /* ���յ���0X0D */
            {
                if (Res != 0x0a)
                {
                    g_usart_rx_sta = 0;                      /* ���մ���,���¿�ʼ */
                }
                else
                {
                    g_usart_rx_sta |= 0x8000;                /* ��������� */
                }
            }
            else                                             /* ��û�յ�0X0D */
            {
                if (Res == 0x0d)
                {
                    g_usart_rx_sta |= 0x4000;
                }
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = Res;
                    g_usart_rx_sta++;

                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;                  /* �������ݴ���,���¿�ʼ���� */
                    }
                }
            }
        }
    }
}
#endif

/**
 * @brief       _write����
 * @param       *buf : Ҫ���͵�����,size: Ҫ���͵����ݳ���
 * @retval      size : ���ݳ���
 */
int _write(int fd, char *buf, int size)
{
    int i;

    for(i=0; i<size; i++)
    {
        while (USART_GetFlagStatus(USART_UX, USART_FLAG_TC) == RESET);
        USART_SendData(USART_UX, *buf++);
    }
    return size;
}

