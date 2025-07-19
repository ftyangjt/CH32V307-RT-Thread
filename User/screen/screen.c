/********************************** (C) COPYRIGHT *******************************
* File Name          : screen.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/06/27
* Description        : ��Ļ��ʾģ��ʵ���ļ�
*********************************************************************************/

#include "screen.h"
#include "../Cardinal.h"

/**
 * @brief       ��ʾιʳ״̬����
 * @param       ��
 * @retval      ��
 */
void display_feeding_ui(void)
{
    // ��Ļ�м���ʾ"FEEDING"
    // ��Ļ�ߴ� 480x320��������ʾ
    text_show_string(170, 140, 140, 48, "FEEDING", 48, 1, LIGHT_BG);
}

/**
 * @brief       ��ʾ���״̬UI����
 * @param       ��
 * @retval      ��
 */
void display_aquarium_ui(void)
{   
    // �����Ļ
    lcd_clear(BLACK);
    
    // ���ر�����ʾ����ͼƬ
    char bg_path[] = "0:/PICTURE/background.bmp";
    uint8_t res = piclib_ai_load_picfile(bg_path, 0, 0, lcddev.width, lcddev.height, 1);
    
    if (res != 0) 
    {
        // ��ʾ������ʾ����ѡ��
        text_show_string(10, 10, 200, 16, "����ͼƬ����ʧ��", 16, 1, RED);
    }

    text_show_string(10, 10, 150, 48, "10:32", 48, 1, BLACK);

    text_show_string(240, 10, 150, 24, "��ǰ�¶�: ", 24, 1, BLACK);

    text_show_string(350, 10, 150, 48, "66.66", 48, 1, BLACK);

    text_show_string(190, 70, 100, 32, "�´�ιʳʱ��", 24, 1, BLACK);

    text_show_string(200, 100, 100, 48, "3:00", 32, 1, BLACK);

}

/**
 * @brief       ���״̬��ʾ�߳���ں���
 * @param       parameter : �̲߳���
 * @retval      ��
 */
void pic_show_thread_entry(void *parameter)
{
    // ��ʼ��ͼƬ��
    piclib_init();

    display_aquarium_ui();

    // ��ʼʱ�䣨�ɸ���ʵ������޸ģ�
    int hour = 10, min = 25, sec = 0;
    
    // ��¼��һ�ε�ιʳ״̬
    int last_feeding_state = 0;

    while (1)
    {   
        // ���ιʳ״̬�Ƿ����仯
        if (onFeeding != last_feeding_state)
        {
            last_feeding_state = onFeeding;
            
            if (onFeeding)
            {
                // �л�����ɫ���䱳������ʾιʳ����
                display_feeding_ui();
            }
            else
            {
                // �ָ���׽���
                display_aquarium_ui();
            }
        }
        
        if (onFeeding)
        {
            // ιʳ״̬��ֻ���ֺ�ɫ���䱳����������������Ϣ
        }
        else
        {
            // ��̬ʱ����ʾ
            update_time_display(hour, min, sec);

            // ��̬�¶���ʾ
            update_temperature_display();

            // ʱ�����
            sec++;
            if (sec >= 60)
            {
                sec = 0;
                min++;
                if (min >= 60)
                {
                    min = 0;
                    hour++;
                    if (hour >= 24)
                        hour = 0;
                }
            }
        }
        
        rt_thread_mdelay(1000);
    }
}

// void pic_show_thread_entry(void *parameter)
// {
//     // ��ʼ��ͼƬ��
//     piclib_init();

//     display_aquarium_ui();
// }

void update_time_display(int hour, int min, int sec)
{
    char time_str[16];
    rt_snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, min, sec);

    // ���ñ���ɫ���ʱ����ʾ����
    lcd_fill(10, 10, 10+150, 10+48, BG_TOP);
    text_show_string(10, 10, 150, 48, time_str, 48, 1, BLACK);

}

void update_temperature_display(void)
{
    int temp_raw = adc_get_temperature(); // ���� 2534 ��ʾ 25.34��C
    int temp_int = temp_raw / 100;
    int temp_frac = temp_raw % 100;
    char temp_str[16];
    rt_snprintf(temp_str, sizeof(temp_str), "%d.%02d��C", temp_int, temp_frac);

    // ���ñ���ɫ����¶���ʾ����
    lcd_fill(350, 10, 350+150, 10+48, BG_BOTTOM);
    
    text_show_string(350, 10, 150, 48, temp_str, 48, 1, BLACK);
}

/**
 * @brief       ��ʼ����Ļģ�鲢����UI��ʾ�߳�
 * @param       ��
 * @retval      �߳̾��
 */
rt_thread_t screen_init(void)
{
    my_mem_init(SRAMIN);                                /* ��ʼ���ڲ�SRAM�ڴ�� */
    exfuns_init();                                      /* Ϊfatfs��������ڴ� */
    f_mount(fs[0], "0:", 1);                            /* ����SD�� */
    f_mount(fs[1], "1:", 1);                            /* ����FLASH */

    while (fonts_init())                                /* ����ֿ� */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        rt_thread_mdelay(200);
        lcd_fill(30, 50, 240, 66, WHITE);               /* �����ʾ */
        rt_thread_mdelay(200);
    }
    
    rt_thread_t ui_tid = rt_thread_create("aquarium_ui",
                                          pic_show_thread_entry,
                                          RT_NULL,
                                          1024,
                                          15,
                                          10);
    if (ui_tid != RT_NULL)
    {
        rt_thread_startup(ui_tid);
    }
    
    return ui_tid;
}
