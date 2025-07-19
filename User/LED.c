#include "ws2812b/rainbow.h"
#include "ws2812b/ws2812.h"
#include <rtthread.h>
#include <stdint.h>
#include "Cardinal.h"
#include <math.h>
#include <stdlib.h> // ���stdlib.h����atoi


static rt_thread_t breathing_thread = RT_NULL;

static struct {
    uint8_t r;
    uint8_t g; 
    uint8_t b;
    uint8_t max_brightness;  // ������Ȱٷֱ�(0-100)
    uint32_t cycle_time_ms;  // ��������ʱ��(����)
} breathing_config = {255, 255, 255, 100, 3000};  // Ĭ�ϰ�ɫ��100%������ȣ�3������
;
extern struct rt_semaphore *g_breathing_sem;
extern struct rt_semaphore *g_breathing_done_sem;
/* ������Ч���߳� */
static void breathing_thread_entry()
{
    
    uint8_t color_data[WS2812_LED_NUM * 3];
    //ʹ�����Ҳ����㵱ǰ���Ȱٷֱ�(0-max_brightness)
    float brightness_factor = 0.0f;
    uint8_t current_brightness = 0;
    uint8_t r, g, b;
    
    while (1) {
        //��Զ�ȴ������߳��ź���
        rt_sem_take(g_breathing_sem, RT_WAITING_FOREVER);
        rainbow_stop();
        uint32_t run_time = 0;
        if(g_light == 1){
            breathing_config.r = 255;
            breathing_config.g = 255;
            breathing_config.b = 255;
            breathing_config.max_brightness = 100;
            breathing_config.cycle_time_ms = 3000;
            run_time = 2000;
        }
        if(g_light == 2){
            breathing_config.r = 255;
            breathing_config.g = 150;
            breathing_config.b = 150;
            breathing_config.max_brightness = 100;
            breathing_config.cycle_time_ms = 1350;
            run_time = 5000;
        }        
        if(g_light == 3){
            breathing_config.r = 150;
            breathing_config.g = 255;
            breathing_config.b = 150;
            breathing_config.max_brightness = 100;
            breathing_config.cycle_time_ms = 3000;
        }        
        if(g_light == 4){
            breathing_config.r = 255;
            breathing_config.g = 0;
            breathing_config.b = 0;
            breathing_config.max_brightness = 100;
            breathing_config.cycle_time_ms = 3000;
        }
        float angle = 0.0f;
        float step = 2 * PI / (breathing_config.cycle_time_ms / 20.0f); // ÿ20ms����һ�Σ����㲽��

        rt_kprintf("�ƹ��߳�����\n");
        while (run_time < 1000 * 10) { // ����5��
            // ʹ�����Ҳ����㵱ǰ���Ȱٷֱ�(0-max_brightness)
            brightness_factor = (sinf(angle) + 1.0f) / 2.0f;  // ��-1��1������ֵӳ�䵽0��1
            current_brightness = (uint8_t)(breathing_config.max_brightness * brightness_factor);
            
            // �������ȵ���RGBֵ
            r = (uint8_t)((breathing_config.r * current_brightness) / 100);
            g = (uint8_t)((breathing_config.g * current_brightness) / 100);
            b = (uint8_t)((breathing_config.b * current_brightness) / 100);
            
            // �������LED
            for (int i = 0; i < WS2812_LED_NUM; i++) {
                color_data[i * 3 + 0] = r; // R
                color_data[i * 3 + 1] = g; // G
                color_data[i * 3 + 2] = b; // B
            }
            
            
            ws2812_update(color_data);
            
            // ���½Ƕ�
            angle += step;
            if (angle >= 2 * PI) {
                angle -= 2 * PI;
            }
            
            rt_thread_mdelay(20); // 20msˢ��һ��
            run_time += 20;
        }

        // �ر�����LED
        for (int i = 0; i < WS2812_LED_NUM * 3; i++) color_data[i] = 0;
        ws2812_update(color_data);

        // ֪ͨ�����̵߳ƹ������
        rainbow_start(1);
        rt_sem_release(g_breathing_done_sem);
    }
}

/* ����������Ч�� */
void breathing_start()
{
    breathing_thread = rt_thread_create("breathing",
                                        breathing_thread_entry,
                                        RT_NULL,
                                        1024,
                                        7,
                                        10);
    if (breathing_thread != RT_NULL) {
        rt_thread_startup(breathing_thread);
        rt_kprintf("������Ч��������\n");
    } else {
        rt_kprintf("�������̴߳���ʧ�ܣ�\n");
    }
}
















/* ���ú�������ɫ��������� */
void breathing_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t max_brightness)
{
    breathing_config.r = r;
    breathing_config.g = g;
    breathing_config.b = b;
    
    if (max_brightness > 100) max_brightness = 100;
    breathing_config.max_brightness = max_brightness;
    
    rt_kprintf("��������ɫ������ΪRGB:(%d,%d,%d)���������:%d%%\n", r, g, b, max_brightness);
}

/* MSH�������������Ч�� */
static int cmd_breathing(int argc, char **argv)
{
    int cycle_time = 3; // Ĭ��3������
    
    if (argc > 1) {
        cycle_time = str_to_int(argv[1]);
    }
    
    breathing_start(cycle_time);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_breathing, start breathing effect [cycle_time_seconds]);


/* MSH������ú�������ɫ������ */
static int cmd_breathing_color(int argc, char **argv)
{
    if (argc < 4) {
        rt_kprintf("�÷�: breathing_color r g b [max_brightness]\n");
        rt_kprintf("ʾ��: breathing_color 255 0 0 80  ����Ϊ��ɫ���������80%%\n");
        return -RT_ERROR;
    }
    
    int r = str_to_int(argv[1]);
    int g = str_to_int(argv[2]);
    int b = str_to_int(argv[3]);
    
    // ���RGB��Χ
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    
    int max_brightness = 100; // Ĭ���������
    if (argc > 4) {
        max_brightness = str_to_int(argv[4]);
        if (max_brightness < 0) max_brightness = 0;
        if (max_brightness > 100) max_brightness = 100;
    }
    
    breathing_set_color(r, g, b, max_brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_breathing_color, set breathing color: r g b [max_brightness]);

