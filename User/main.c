/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : ChatGPT
* Version            : V2.0.0
* Date               : 2025/05/27
* Description        : CH32V307 + RT-Thread 主程序
*********************************************************************************/

#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>
#include "drivers/pin.h"

// 包含模块头文件
#include "ws2812b/ws2812.h"
#include "ws2812b/rainbow.h"

// 包含屏幕头文件
#include "BSP/LCD/lcd.h"

// 包含SD卡相关头文件
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SDIO/sdio_sdcard.h"
#include "./BSP/NORFLASH/norflash.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./MALLOC/malloc.h"
#include "./TEXT/text.h"
#include "./PICTURE/piclib.h"
#include "string.h"
#include "math.h"

#include "WIFI.h"
#include "PWM.h"
#include "drv_pwm.h"
#include "shell.h"

// 自定义字符串转整数函数
static int str_to_int(const char* str)
{
    int result = 0;
    int sign = 1;
    
    // 处理符号
    if(*str == '-') {
        sign = -1;
        str++;
    } else if(*str == '+') {
        str++;
    }
    
    // 处理数字
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

// MSH命令：启动彩虹效果
static int cmd_rainbow(int argc, char **argv)
{
    int speed = 3;  // 默认速度级别
    
    if(argc > 1) {
        speed = str_to_int(argv[1]);
    }
    
    rainbow_start(speed);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow, start rainbow effect [speed 1-5]);

// MSH命令：停止彩虹效果
static int cmd_rainbow_stop(int argc, char **argv)
{
    rainbow_stop();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_stop, stop rainbow effect);

// MSH命令：设置彩虹效果亮度
static int cmd_brightness(int argc, char **argv)
{
    int brightness = 30;  // 默认亮度30%
    
    if(argc > 1) {
        brightness = str_to_int(argv[1]);
        if(brightness < 0) brightness = 0;
        if(brightness > 100) brightness = 100;
    }
    
    rainbow_set_brightness(brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_brightness, set rainbow brightness [0-100]);

/**
 * @brief       得到path路径下,目标文件的总个数
 * @param       path : 路径
 * @retval      总有效文件数
 */
uint16_t pic_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    DIR tdir;                                                   /* 临时目录 */
    FILINFO *tfileinfo;                                         /* 临时文件信息 */
    tfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));   /* 申请内存 */
    res = f_opendir(&tdir, (const TCHAR *)path);                /* 打开目录 */

    if (res == FR_OK && tfileinfo)
    {
        while (1)                                               /* 查询总的有效文件数 */
        {
            res = f_readdir(&tdir, tfileinfo);                  /* 读取目录下的一个文件 */

            if (res != FR_OK || tfileinfo->fname[0] == 0)
            {
                break;                                          /* 错误了/到末尾了,退出 */
            }

            res = exfuns_file_type(tfileinfo->fname);

            if ((res & 0XF0) == 0X50)                           /* 取高四位,看看是不是图片文件 */
            {
                rval++;                                         /* 有效文件数增加1 */
            }
        }
    }

    myfree(SRAMIN, tfileinfo);                                  /* 释放内存 */
    return rval;
}

static void pic_show_thread_entry(void *parameter)
{
    DIR picdir;
    FILINFO *picfileinfo;
    char *pname;
    uint16_t totpicnum;
    uint16_t curindex;
    uint8_t key;
    uint8_t pause = 0;
    uint8_t t;
    uint16_t temp;
    uint32_t *picoffsettbl;
    int res;

    // 打开图片目录
    while (f_opendir(&picdir, "0:/PICTURE"))
    {
        text_show_string(30, 150, 240, 16, "PICTURE文件夹错误!", 16, 0, RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);
        rt_thread_mdelay(200);
    }

    totpicnum = pic_get_tnum("0:/PICTURE"); // 获取有效图片数

    while (totpicnum == 0)
    {
        text_show_string(30, 150, 240, 16, "没有图片文件!", 16, 0, RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);
        rt_thread_mdelay(200);
    }

    picfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));
    pname = mymalloc(SRAMIN, FF_MAX_LFN * 2 + 1);
    picoffsettbl = mymalloc(SRAMIN, 4 * totpicnum);

    while (!picfileinfo || !pname || !picoffsettbl)
    {
        text_show_string(30, 150, 240, 16, "内存分配失败!", 16, 0, RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);
        rt_thread_mdelay(200);
    }

    // 记录所有图片文件的目录偏移
    res = f_opendir(&picdir, "0:/PICTURE");
    if (res == FR_OK)
    {
        curindex = 0;
        while (1)
        {
            temp = picdir.dptr;
            res = f_readdir(&picdir, picfileinfo);
            if (res != FR_OK || picfileinfo->fname[0] == 0)
            {
                break;
            }
            res = exfuns_file_type(picfileinfo->fname);
            if ((res & 0XF0) == 0X50)
            {
                picoffsettbl[curindex] = temp;
                curindex++;
            }
        }
    }

    text_show_string(30, 150, 240, 16, "开始显示...", 16, 0, RED);
    rt_thread_mdelay(1500);
    piclib_init();
    curindex = 0;
    res = f_opendir(&picdir, (const TCHAR *)"0:/PICTURE");

    while (res == FR_OK)
    {
        dir_sdi(&picdir, picoffsettbl[curindex]);
        res = f_readdir(&picdir, picfileinfo);
        if (res != FR_OK || picfileinfo->fname[0] == 0) break;

        strcpy((char *)pname, "0:/PICTURE/");
        strcat((char *)pname, (const char *)picfileinfo->fname);
        lcd_clear(BLACK);

        piclib_ai_load_picfile(pname, 0, 0, lcddev.width, lcddev.height, 1);
        t = 0;

        while (1)
        {
            key = key_scan(0);

            if (t > 250)
                key = 1; // 自动下一张

            if ((t % 20) == 0)
            {
                LED0_TOGGLE();
            }

            if (key == KEY1_PRES)
            {
                if (curindex)
                {
                    curindex--;
                }
                else
                {
                    curindex = totpicnum - 1;
                }
                break;
            }
            else if (key == KEY0_PRES)
            {
                curindex++;
                if (curindex >= totpicnum)
                    curindex = 0;
                break;
            }
            else if (key == WKUP_PRES)
            {
                pause = !pause;
                LED1(!pause);
            }

            if (pause == 0)
                t++;

            rt_thread_mdelay(10);
        }
        res = 0;
    }

    myfree(SRAMIN, picfileinfo);
    myfree(SRAMIN, pname);
    myfree(SRAMIN, picoffsettbl);

    // 线程结束后挂起自己
    rt_thread_suspend(rt_thread_self());
    rt_schedule();
}

int main(void)
{
    rt_kprintf("\r\n");
    rt_kprintf("******************************************************\r\n");
    rt_kprintf("* MCU: CH32V307                                      *\r\n");
    rt_kprintf("* System: V2.0.0                                     *\r\n");
    rt_kprintf("* Date: 2025/05/27                                   *\r\n");
    rt_kprintf("******************************************************\r\n");
    
    SystemCoreClockUpdate();
    rt_kprintf("SysClk: %dHz\r\n", SystemCoreClock);
    rt_kprintf("ChipID: %08x\r\n", DBGMCU_GetCHIPID());

    // 初始化屏幕
    lcd_init();

    // 初始化彩虹灯光模块
    ws2812_init();
    rainbow_init();
    rainbow_start(3);  // 启动彩虹效果，速度级别3

    my_mem_init(SRAMIN);                                /* 初始化内部SRAM内存池 */
    exfuns_init();                                      /* 为fatfs相关变量申请内存 */
    f_mount(fs[0], "0:", 1);                            /* 挂载SD卡 */
    f_mount(fs[1], "1:", 1);                            /* 挂载FLASH */

    while (fonts_init())                                /* 检查字库 */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 50, 240, 66, WHITE);               /* 清除显示 */
        rt_thread_mdelay(200);
    }

    text_show_string(30,  50, 200, 16, "正点原子CH32开发板", 16, 0, RED);
    text_show_string(30,  70, 200, 16, "图片显示 实验", 16, 0, RED);
    text_show_string(30,  90, 200, 16, "KEY0:NEXT KEY1:PREV", 16, 0, RED);
    text_show_string(30, 110, 200, 16, "KEY_UP:PAUSE", 16, 0, RED);
    text_show_string(30, 130, 200, 16, "正点原子@ALIENTEK", 16, 0, RED);
        pwm_module_init();  
    wifi_module_init(); 

    rt_thread_t pic_tid = rt_thread_create("picshow",
                                          pic_show_thread_entry,
                                          RT_NULL,
                                          2048,
                                          15,
                                          10);
    if (pic_tid != RT_NULL)
        rt_thread_startup(pic_tid);

    // 主循环
    while(1)
    {
        rt_thread_mdelay(1000);
        // 主循环保持空闲，工作由线程完成
    }
}