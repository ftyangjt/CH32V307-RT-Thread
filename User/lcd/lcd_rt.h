#ifndef __LCD_RT_H__
#define __LCD_RT_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

/* 硬件抽象接口声明 */
void lcd_hw_init(void);
void lcd_hw_clear(uint16_t color);
void lcd_hw_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_hw_show_string(uint16_t x, uint16_t y, const char *str, uint16_t color);

/* 设备驱动初始化函数声明 */
int lcd_drv_init(void);

#endif /* __LCD_RT_H__ */