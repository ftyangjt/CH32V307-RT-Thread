/**
 * @file        lcd_rtt.c
 * @brief       LCD的RT-Thread设备驱动实现
 * @author      RT-Thread Team
 * @version     v1.0
 * @date        2025-06-13
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "./BSP/LCD/lcd.h"
#include "./BSP/LCD/lcd_rtt.h"

/* LCD设备结构体实例 */
static struct lcd_device lcd_dev;

/* LCD信息结构体 */
struct rt_device_graphic_info lcd_info;

/**
 * @brief LCD设备初始化函数
 */
static rt_err_t rt_lcd_init(rt_device_t dev)
{
    /* 调用底层LCD初始化函数 */
    lcd_init();
    
    return RT_EOK;
}

/**
 * @brief LCD设备打开函数
 */
static rt_err_t rt_lcd_open(rt_device_t dev, rt_uint16_t oflag)
{
    /* 打开LCD显示 */
    lcd_display_on();
    
    return RT_EOK;
}

/**
 * @brief LCD设备关闭函数
 */
static rt_err_t rt_lcd_close(rt_device_t dev)
{
    /* 关闭LCD显示 */
    lcd_display_off();
    
    return RT_EOK;
}

/**
 * @brief LCD设备读取函数(读取像素点)
 */
static rt_size_t rt_lcd_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_uint16_t x = pos & 0xFFFF;
    rt_uint16_t y = (pos >> 16) & 0xFFFF;
    rt_uint32_t *color = (rt_uint32_t *)buffer;
    
    if (x >= lcd_dev.width || y >= lcd_dev.height)
        return 0;
    
    *color = lcd_read_point(x, y);
    
    return sizeof(rt_uint32_t);
}

/**
 * @brief LCD设备写入函数(写入像素点)
 */
static rt_size_t rt_lcd_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_uint16_t x = pos & 0xFFFF;
    rt_uint16_t y = (pos >> 16) & 0xFFFF;
    rt_uint32_t color = *(rt_uint32_t *)buffer;
    
    if (x >= lcd_dev.width || y >= lcd_dev.height)
        return 0;
    
    lcd_draw_point(x, y, color);
    
    return sizeof(rt_uint32_t);
}

/**
 * @brief LCD设备控制函数
 */
static rt_err_t rt_lcd_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd)
    {
    case RTGRAPHIC_CTRL_GET_INFO:
        /* 获取LCD信息 */
        lcd_info.bits_per_pixel = 16;         /* 每像素16位 */
        lcd_info.pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        lcd_info.width = lcddev.width;
        lcd_info.height = lcddev.height;
        lcd_info.framebuffer = RT_NULL;       /* 没有帧缓冲区 */
        
        rt_memcpy(args, &lcd_info, sizeof(lcd_info));
        break;
    
    case RTGRAPHIC_CTRL_RECT_UPDATE:
        /* 更新区域，当前不支持局部更新 */
        break;
    
    case RTGRAPHIC_CTRL_POWERON:
        /* 打开LCD显示 */
        lcd_display_on();
        break;
    
    case RTGRAPHIC_CTRL_POWEROFF:
        /* 关闭LCD显示 */
        lcd_display_off();
        break;
    
    case RTGRAPHIC_CTRL_SET_BRIGHTNESS:
        /* 设置背光亮度 */
        if (lcddev.id == 0x1963)
        {
            lcd_ssd_backlight_set(*(rt_uint8_t*)args);
        }
        else
        {
            /* 其他屏幕可能需要通过PWM等方式控制 */
        }
        break;
    
    case RTGRAPHIC_CTRL_GET_BRIGHTNESS:
        /* 获取背光亮度 - 当前不支持 */
        *(rt_uint8_t*)args = 100;  /* 默认返回100% */
        break;
    
    case RTGRAPHIC_CTRL_SET_DIRECTION:
        /* 设置显示方向 */
        lcd_display_dir(*(rt_uint8_t*)args);
        lcd_dev.width = lcddev.width;
        lcd_dev.height = lcddev.height;
        lcd_dev.dir = lcddev.dir;
        break;
    
    case RTGRAPHIC_CTRL_GET_DIRECTION:
        /* 获取显示方向 */
        *(rt_uint8_t*)args = lcd_dev.dir;
        break;
    
    default:
        return -RT_ERROR;
    }
    
    return RT_EOK;
}

/**
 * @brief 绘制屏幕函数，适配RT-Thread GUI
 */
static void lcd_draw_hline_buffer(const char *pixel, int x1, int x2, int y)
{
    const rt_uint16_t *ptr = (rt_uint16_t*)pixel;
    
    /* 绘制一条水平线 */
    lcd_set_cursor(x1, y);
    lcd_write_ram_prepare();
    
    for (int i = x1; i <= x2; i++)
    {
        LCD->LCD_RAM = *ptr++;
    }
}

/**
 * @brief 初始化LCD的RT-Thread设备
 */
int rt_hw_lcd_init(void)
{
    /* 获取LCD信息 */
    lcd_dev.width = lcddev.width;
    lcd_dev.height = lcddev.height;
    lcd_dev.dir = lcddev.dir;
    lcd_dev.id = lcddev.id;
    lcd_dev.fb = RT_NULL;  /* 无帧缓冲 */
    
    /* 设置设备类型和操作接口 */
    lcd_dev.parent.type = RT_Device_Class_Graphic;
    lcd_dev.parent.rx_indicate = RT_NULL;
    lcd_dev.parent.tx_complete = RT_NULL;
    
    /* 设置设备接口函数 */
    lcd_dev.parent.init = rt_lcd_init;
    lcd_dev.parent.open = rt_lcd_open;
    lcd_dev.parent.close = rt_lcd_close;
    lcd_dev.parent.read = rt_lcd_read;
    lcd_dev.parent.write = rt_lcd_write;
    lcd_dev.parent.control = rt_lcd_control;
    lcd_dev.parent.user_data = RT_NULL;
    
    /* 注册设备到RT-Thread */
    rt_device_register(&lcd_dev.parent, "lcd", RT_DEVICE_FLAG_RDWR);
    
    /* 如果使用RT-Thread GUI，设置绘图操作接口 */
#ifdef RT_USING_GUIENGINE
    rtgui_graphic_set_device(&lcd_dev.parent);
#endif
    
    return 0;
}

/* 使用自动初始化 */
INIT_DEVICE_EXPORT(rt_hw_lcd_init);