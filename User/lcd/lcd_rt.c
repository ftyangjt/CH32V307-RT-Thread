#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>
#include "./BSP/LCD/lcd.h"
#include "lcd_rt.h"

void lcd_hw_init(void)
{
    lcd_init();
}

void lcd_hw_clear(uint16_t color)
{
    lcd_clear(color);
}

void lcd_hw_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);
}

void lcd_hw_show_string(uint16_t x, uint16_t y, const char *str, uint16_t color)
{
    // 假设字体大小为16，宽度和高度为屏幕剩余空间，可根据实际需求调整
    lcd_show_string(x, y, lcddev.width - x, lcddev.height - y, 16, (char *)str, color);
}

#define LCD_WIDTH   320
#define LCD_HEIGHT  480

static struct rt_device lcd_device;

static rt_err_t lcd_rt_init(rt_device_t dev)
{
    lcd_hw_init();
    return RT_EOK;
}

static rt_err_t lcd_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t lcd_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t lcd_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    // 一般LCD不支持读
    return 0;
}

static rt_size_t lcd_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    // 这里假设buffer为一组像素数据，用户可自定义协议
    // 这里只做演示
    return 0;
}

static rt_err_t lcd_control(rt_device_t dev, int cmd, void *args)
{
    switch(cmd)
    {
    case 0: // 清屏
        lcd_hw_clear(*(uint16_t*)args);
        break;
    case 1: // 画点
    {
        struct
        {
            uint16_t x, y, color;
        } *pt = args;
        lcd_hw_draw_point(pt->x, pt->y, pt->color);
        break;
    }
    case 2: // 显示字符串
    {
        struct
        {
            uint16_t x, y;
            const char *str;
            uint16_t color;
        } *pt = args;
        lcd_hw_show_string(pt->x, pt->y, pt->str, pt->color);
        break;
    }
    default:
        break;
    }
    return RT_EOK;
}

int lcd_drv_init(void)
{
    lcd_device.type        = RT_Device_Class_Graphic;
    lcd_device.init        = lcd_rt_init;
    lcd_device.open        = lcd_open;
    lcd_device.close       = lcd_close;
    lcd_device.read        = lcd_read;
    lcd_device.write       = lcd_write;
    lcd_device.control     = lcd_control;
    lcd_device.user_data   = RT_NULL;

    /* 注册设备 */
    rt_device_register(&lcd_device, "lcd", RT_DEVICE_FLAG_RDWR);
    return 0;
}
INIT_DEVICE_EXPORT(lcd_drv_init);