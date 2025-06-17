/**
 ****************************************************************************************************
 * @file        spi.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       SPI ��������
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

#include "./BSP/SPI/spi.h"


/**
 * @brief       SPI��ʼ������
 *   @note      ����ģʽ,8λ����,��ֹӲ��Ƭѡ
 * @param       ��
 * @retval      ��
 */
void spi2_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    SPI_InitTypeDef  spi2_handle;

    SPI2_SPI_CLK_ENABLE();       /* SPI2ʱ��ʹ�� */
    SPI2_SCK_GPIO_CLK_ENABLE();  /* SPI2_SCK��ʱ��ʹ�� */
    SPI2_MISO_GPIO_CLK_ENABLE(); /* SPI2_MISO��ʱ��ʹ�� */
    SPI2_MOSI_GPIO_CLK_ENABLE(); /* SPI2_MOSI��ʱ��ʹ�� */

    /* SCK����ģʽ����(�������) */
    gpio_init_struct.GPIO_Pin = SPI2_SCK_GPIO_PIN;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI2_SCK_GPIO_PORT, &gpio_init_struct);

    /* MISO����ģʽ����(�������) */
    gpio_init_struct.GPIO_Pin = SPI2_MISO_GPIO_PIN;
    GPIO_Init(SPI2_MISO_GPIO_PORT, &gpio_init_struct);

    /* MOSI����ģʽ����(�������) */
    gpio_init_struct.GPIO_Pin = SPI2_MOSI_GPIO_PIN;
    GPIO_Init(SPI2_MOSI_GPIO_PORT, &gpio_init_struct);

    spi2_handle.SPI_Mode = SPI_Mode_Master;                                 /* ����SPI����ģʽ������Ϊ��ģʽ */
    spi2_handle.SPI_Direction = SPI_Direction_2Lines_FullDuplex;            /* ����SPI�������˫�������ģʽ:SPI����Ϊ˫��ģʽ */
    spi2_handle.SPI_DataSize = SPI_DataSize_8b;                             /* ����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ */
    spi2_handle.SPI_CPOL = SPI_CPOL_High;                                   /* ����ͬ��ʱ�ӵĿ���״̬Ϊ�ߵ�ƽ */
    spi2_handle.SPI_CPHA = SPI_CPHA_2Edge;                                  /* ����ͬ��ʱ�ӵĵڶ��������أ��������½������ݱ����� */
    spi2_handle.SPI_NSS = SPI_NSS_Soft;                                     /* NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ���� */
    spi2_handle.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;          /* ���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256 */
    spi2_handle.SPI_FirstBit = SPI_FirstBit_MSB;                            /* ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ */
    spi2_handle.SPI_CRCPolynomial = 7;                                      /* CRCֵ����Ķ���ʽ */
    SPI_Init(SPI2_SPI,&spi2_handle);                                        /* ��ʼ�� */

    SPI_Cmd(SPI2_SPI, ENABLE);  /* ʹ��SPI2 */

    spi2_read_write_byte(0Xff); /* ��������, ʵ���Ͼ��ǲ���8��ʱ������, �ﵽ���DR������, �Ǳ��� */
}

/**
 * @brief       SPI2�ٶ����ú���
 *   @note      SPI2ʱ��ѡ������APB1, ��PCLK1, Ϊ72Mhz
 *              SPI�ٶ� = PCLK1 / 2^(speed + 1)
 * @param       speed   : SPI2ʱ�ӷ�Ƶϵ��
                                ȡֵΪSPI_SPEED_2~SPI_SPEED_256��0~7��
 * @retval      ��
 */
void spi2_set_speed(uint8_t speed)
{
    speed &= 0X07;                      /* ���Ʒ�Χ */
    SPI_Cmd(SPI2_SPI,DISABLE);          /* �ر�SPI */
    SPI2_SPI->CTLR1 &= 0XFFC7;          /* λ3-5���㣬�������ò����� */
    SPI2_SPI->CTLR1 |= speed << 3;      /* ����SPI�ٶ� */
    SPI_Cmd(SPI2_SPI,ENABLE);           /* ʹ��SPI */
}

/**
 * @brief       SPI2��дһ���ֽ�����
 * @param       txdata  : Ҫ���͵�����(1�ֽ�)
 * @retval      ���յ�������(1�ֽ�)
 */
uint8_t spi2_read_write_byte(uint8_t txdata)
{
    while (SPI_I2S_GetFlagStatus(SPI2_SPI, SPI_I2S_FLAG_TXE) == RESET);  /* �ȴ��������� */

    SPI_I2S_SendData(SPI2_SPI, txdata);                                  /* ����һ��byte */

    while (SPI_I2S_GetFlagStatus(SPI2_SPI, SPI_I2S_FLAG_RXNE) == RESET); /* �ȴ�������һ��byte */

    return SPI_I2S_ReceiveData(SPI2_SPI);                                /* �����յ������� */
}


