/**
 ****************************************************************************************************
 * @file        lcd_ex.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-07-20
 * @brief       各个LCD驱动IC的寄存器初始化部分代码,以简化lcd.c
 * 
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
 ****************************************************************************************************
 */

#ifndef __LCD_EX_H
#define __LCD_EX_H

#include "stdlib.h"
#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/

void lcd_ex_st7789_reginit(void);       /* ST7789寄存器初始化代码 */
void lcd_ex_ili9341_reginit(void);      /* ILI9341寄存器初始化代码 */
void lcd_ex_nt35310_reginit(void);      /* NT35310寄存器初始化代码 */
void lcd_ex_nt35510_reginit(void);      /* NT35510寄存器初始化代码 */
void lcd_ex_st7796_reginit(void);       /* ST7796寄存器初始化代码 */
void lcd_ex_ssd1963_reginit(void);      /* SSD1963寄存器初始化代码 */
void lcd_ex_ili9806_reginit(void);      /* ILI9806寄存器初始化代码 */

#endif

