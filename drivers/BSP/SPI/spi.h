/**
 ****************************************************************************************************
 * @file        spi.h
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

#ifndef __SPI_H
#define __SPI_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/

/* SPI2���� ���� */
#define SPI2_SCK_GPIO_PORT              GPIOB
#define SPI2_SCK_GPIO_PIN               GPIO_Pin_13
#define SPI2_SCK_GPIO_CLK_ENABLE()      do{ RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); }while(0)   /* PB��ʱ��ʹ�� */

#define SPI2_MISO_GPIO_PORT             GPIOB
#define SPI2_MISO_GPIO_PIN              GPIO_Pin_14
#define SPI2_MISO_GPIO_CLK_ENABLE()     do{ RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); }while(0)   /* PB��ʱ��ʹ�� */

#define SPI2_MOSI_GPIO_PORT             GPIOB
#define SPI2_MOSI_GPIO_PIN              GPIO_Pin_15
#define SPI2_MOSI_GPIO_CLK_ENABLE()     do{ RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); }while(0)   /* PB��ʱ��ʹ�� */

/* SPI2��ض��� */
#define SPI2_SPI                        SPI2
#define SPI2_SPI_CLK_ENABLE()           do{ RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); }while(0)    /* SPI2ʱ��ʹ�� */

/******************************************************************************************/

/* SPI�����ٶ����� */
#define SPI_SPEED_2         0
#define SPI_SPEED_4         1
#define SPI_SPEED_8         2
#define SPI_SPEED_16        3
#define SPI_SPEED_32        4
#define SPI_SPEED_64        5
#define SPI_SPEED_128       6
#define SPI_SPEED_256       7

/******************************************************************************************/

void spi2_init(void);                           /* SPI2��ʼ�� */
void spi2_set_speed(uint8_t speed);             /* SPI2�ٶ����� */
uint8_t spi2_read_write_byte(uint8_t txdata);   /* SPI2��д�ֽ� */

#endif

