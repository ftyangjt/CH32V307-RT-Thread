/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : ChatGPT
* Version            : V2.0.0
* Date               : 2025/05/27
* Description        : CH32V307 + RT-Thread ������
*********************************************************************************/

#include "ch32v30x.h"
#include <rtthread.h>
#include <rthw.h>
#include "drivers/pin.h"

// ����ģ��ͷ�ļ�
#include "ws2812b/ws2812.h"
#include "ws2812b/rainbow.h"

// ������Ļͷ�ļ�
#include "BSP/LCD/lcd.h"

// ����SD�����ͷ�ļ�
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

// �Զ����ַ���ת��������
static int str_to_int(const char* str)
{
    int result = 0;
    int sign = 1;
    
    // �������
    if(*str == '-') {
        sign = -1;
        str++;
    } else if(*str == '+') {
        str++;
    }
    
    // ��������
    while(*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

// MSH��������ʺ�Ч��
static int cmd_rainbow(int argc, char **argv)
{
    int speed = 3;  // Ĭ���ٶȼ���
    
    if(argc > 1) {
        speed = str_to_int(argv[1]);
    }
    
    rainbow_start(speed);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow, start rainbow effect [speed 1-5]);

// MSH���ֹͣ�ʺ�Ч��
static int cmd_rainbow_stop(int argc, char **argv)
{
    rainbow_stop();
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_stop, stop rainbow effect);

// MSH������òʺ�Ч������
static int cmd_brightness(int argc, char **argv)
{
    int brightness = 30;  // Ĭ������30%
    
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
 * @brief       �õ�path·����,Ŀ���ļ����ܸ���
 * @param       path : ·��
 * @retval      ����Ч�ļ���
 */
uint16_t pic_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    DIR tdir;                                                   /* ��ʱĿ¼ */
    FILINFO *tfileinfo;                                         /* ��ʱ�ļ���Ϣ */
    tfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));   /* �����ڴ� */
    res = f_opendir(&tdir, (const TCHAR *)path);                /* ��Ŀ¼ */

    if (res == FR_OK && tfileinfo)
    {
        while (1)                                               /* ��ѯ�ܵ���Ч�ļ��� */
        {
            res = f_readdir(&tdir, tfileinfo);                  /* ��ȡĿ¼�µ�һ���ļ� */

            if (res != FR_OK || tfileinfo->fname[0] == 0)
            {
                break;                                          /* ������/��ĩβ��,�˳� */
            }

            res = exfuns_file_type(tfileinfo->fname);

            if ((res & 0XF0) == 0X50)                           /* ȡ����λ,�����ǲ���ͼƬ�ļ� */
            {
                rval++;                                         /* ��Ч�ļ�������1 */
            }
        }
    }

    myfree(SRAMIN, tfileinfo);                                  /* �ͷ��ڴ� */
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

    // ��ͼƬĿ¼
    while (f_opendir(&picdir, "0:/PICTURE"))
    {
        text_show_string(30, 150, 240, 16, "PICTURE�ļ��д���!", 16, 0, RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);
        rt_thread_mdelay(200);
    }

    totpicnum = pic_get_tnum("0:/PICTURE"); // ��ȡ��ЧͼƬ��

    while (totpicnum == 0)
    {
        text_show_string(30, 150, 240, 16, "û��ͼƬ�ļ�!", 16, 0, RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);
        rt_thread_mdelay(200);
    }

    picfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));
    pname = mymalloc(SRAMIN, FF_MAX_LFN * 2 + 1);
    picoffsettbl = mymalloc(SRAMIN, 4 * totpicnum);

    while (!picfileinfo || !pname || !picoffsettbl)
    {
        text_show_string(30, 150, 240, 16, "�ڴ����ʧ��!", 16, 0, RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);
        rt_thread_mdelay(200);
    }

    // ��¼����ͼƬ�ļ���Ŀ¼ƫ��
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

    text_show_string(30, 150, 240, 16, "��ʼ��ʾ...", 16, 0, RED);
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
                key = 1; // �Զ���һ��

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

    // �߳̽���������Լ�
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

    // ��ʼ����Ļ
    lcd_init();

    // ��ʼ���ʺ�ƹ�ģ��
    ws2812_init();
    rainbow_init();
    rainbow_start(3);  // �����ʺ�Ч�����ٶȼ���3

    my_mem_init(SRAMIN);                                /* ��ʼ���ڲ�SRAM�ڴ�� */
    exfuns_init();                                      /* Ϊfatfs��ر��������ڴ� */
    f_mount(fs[0], "0:", 1);                            /* ����SD�� */
    f_mount(fs[1], "1:", 1);                            /* ����FLASH */

    while (fonts_init())                                /* ����ֿ� */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 50, 240, 66, WHITE);               /* �����ʾ */
        rt_thread_mdelay(200);
    }

    text_show_string(30,  50, 200, 16, "����ԭ��CH32������", 16, 0, RED);
    text_show_string(30,  70, 200, 16, "ͼƬ��ʾ ʵ��", 16, 0, RED);
    text_show_string(30,  90, 200, 16, "KEY0:NEXT KEY1:PREV", 16, 0, RED);
    text_show_string(30, 110, 200, 16, "KEY_UP:PAUSE", 16, 0, RED);
    text_show_string(30, 130, 200, 16, "����ԭ��@ALIENTEK", 16, 0, RED);
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

    // ��ѭ��
    while(1)
    {
        rt_thread_mdelay(1000);
        // ��ѭ�����ֿ��У��������߳����
    }
}