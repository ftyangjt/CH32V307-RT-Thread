/**
 ****************************************************************************************************
 * @file        lcd_ex.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       ����LCD����IC�ļĴ�����ʼ�����ִ���,�Լ�lcd.c
 * 
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
 ****************************************************************************************************
 */

#ifndef __LCD_EX_H
#define __LCD_EX_H

#include "stdlib.h"
#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/

void lcd_ex_st7789_reginit(void);       /* ST7789�Ĵ�����ʼ������ */
void lcd_ex_ili9341_reginit(void);      /* ILI9341�Ĵ�����ʼ������ */
void lcd_ex_nt35310_reginit(void);      /* NT35310�Ĵ�����ʼ������ */
void lcd_ex_nt35510_reginit(void);      /* NT35510�Ĵ�����ʼ������ */
void lcd_ex_st7796_reginit(void);       /* ST7796�Ĵ�����ʼ������ */
void lcd_ex_ssd1963_reginit(void);      /* SSD1963�Ĵ�����ʼ������ */
void lcd_ex_ili9806_reginit(void);      /* ILI9806�Ĵ�����ʼ������ */

#endif

