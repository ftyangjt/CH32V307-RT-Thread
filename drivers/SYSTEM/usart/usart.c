/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       串口初始化代码(一般是串口1)，支持printf
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 CH32V307开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20230720
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/usart/usart.h"


#if USART_EN_RX                         /* 如果使能了接收 */

void USART1_IRQHandler_WCH(void) __attribute__((interrupt("WCH-Interrupt-fast")));

uint8_t g_usart_rx_buf[USART_REC_LEN];  /* 接收缓冲, 最大USART_REC_LEN个字节 */

/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart_rx_sta = 0;

#endif

/**
 * @brief       串口X初始化函数
 * @param       无
 * @retval      无
 */
void usart_init(uint32_t bound)
{
    GPIO_InitTypeDef  gpio_init_struct;
    USART_InitTypeDef uartx_init_struct;
    NVIC_InitTypeDef  nvic_init_struct;

    USART_TX_GPIO_CLK_ENABLE();                              /* 使能串口TX脚时钟 */
    USART_RX_GPIO_CLK_ENABLE();                              /* 使能串口RX脚时钟 */
    USART_UX_CLK_ENABLE();                                   /* 使能串口时钟 */

    gpio_init_struct.GPIO_Pin = USART_TX_GPIO_PIN;           /* 串口发送引脚号 */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;            /* 复用推挽输出 */
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;          /* IO速度设置为50M */
    GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.GPIO_Pin = USART_RX_GPIO_PIN;           /* 串口RX脚 模式设置 */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IN_FLOATING;      /* 浮空输入 */
    GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);        /* 串口RX脚,设置成输入模式 */

    /* USART初始化设置*/
    uartx_init_struct.USART_BaudRate = bound;                                       /* 波特率 */
    uartx_init_struct.USART_WordLength = USART_WordLength_8b;                       /* 字长为8位数据格式 */
    uartx_init_struct.USART_StopBits = USART_StopBits_1;                            /* 一个停止位 */
    uartx_init_struct.USART_Parity = USART_Parity_No;                               /* 无奇偶校验位 */
    uartx_init_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   /* 无硬件流控 */
    uartx_init_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                   /* 收发模式 */
    USART_Init(USART_UX,&uartx_init_struct);                                        /* 初始化串口 */

#if USART_EN_RX
    nvic_init_struct.NVIC_IRQChannel = USART_UX_IRQn;        /* 使能USART中断通道 */
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority=3 ;   /* 抢占优先级3 */
    nvic_init_struct.NVIC_IRQChannelSubPriority = 3;         /* 响应优先级3 */
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;            /* IRQ通道使能  */
    NVIC_Init(&nvic_init_struct);                            /* 初始化NVIC */

    USART_ITConfig(USART_UX, USART_IT_RXNE, ENABLE);         /* 开启串口接受中断 */
#endif
    USART_Cmd(USART_UX, ENABLE);                             /* 使能串口 */
}

#if USART_EN_RX                                              /* 如果使能了接收 */
/**
 * @brief       串口X中断服务函数
 * @param       无
 * @retval      无
 */
void USART_UX_IRQHandler(void)
{
    uint8_t Res;

    if(USART_GetITStatus(USART_UX, USART_IT_RXNE) != RESET)  /* 如果是串口x */
    {
        USART_ClearITPendingBit(USART_UX,USART_IT_RXNE);     /* 清除中断标志 */
        Res =USART_ReceiveData(USART_UX);                    /* 读取接收到的数据 */

        if ((g_usart_rx_sta & 0x8000) == 0)                  /* 接收未完成 */
        {
            if (g_usart_rx_sta & 0x4000)                     /* 接收到了0X0D */
            {
                if (Res != 0x0a)
                {
                    g_usart_rx_sta = 0;                      /* 接收错误,重新开始 */
                }
                else
                {
                    g_usart_rx_sta |= 0x8000;                /* 接收完成了 */
                }
            }
            else                                             /* 还没收到0X0D */
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
                        g_usart_rx_sta = 0;                  /* 接收数据错误,重新开始接收 */
                    }
                }
            }
        }
    }
}
#endif

/**
 * @brief       _write函数
 * @param       *buf : 要发送的数据,size: 要发送的数据长度
 * @retval      size : 数据长度
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

