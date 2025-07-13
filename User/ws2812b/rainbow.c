/********************************** (C) COPYRIGHT *******************************
* File Name          : rainbow.c
* Author             : ChatGPT
* Version            : V1.0.0
* Date               : 2025/05/27
* Description        : WS2812�ʺ�ƹ�Ч��
*********************************************************************************/

#include "rainbow.h"
#include "ws2812.h"
#include <math.h>

// ȫ�ֱ���
static rainbow_ctrl_t rainbow_ctrl;
static rt_thread_t rainbow_thread = RT_NULL;
static uint8_t led_data[WS2812_LED_NUM * 3];
static rt_thread_t solid_color_thread = RT_NULL;  // ������Ϊsolid_color_thread
int rainBowType = 1;

// ȫ����ɫ���������ڴ洢��ǰ��ɫģʽ��RGBֵ
static struct {
    uint8_t r;
    uint8_t g; 
    uint8_t b;
    uint8_t brightness;  // ���Ȱٷֱ�(0-100)
} solid_color = {255, 255, 255, 100};  // Ĭ��Ϊ��ɫ��100%����


//�Զ����ַ���ת��������
int str_to_int(const char* str)
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


/* ��ʼ���ʺ�Ч�� */
void rainbow_init(void)
{
    // ��ʼ�����Ʋ���
    rainbow_ctrl.hue = 0.0f;
    rainbow_ctrl.hue_step = 5.0f;
    rainbow_ctrl.delay_ms = 25;
    rainbow_ctrl.brightness = 30;
    rainbow_ctrl.mode = 1;
    
    // ��ʼ������Ч������
    rainbow_ctrl.breathing_enabled = 0;  // Ĭ�Ϲرպ���Ч��
    rainbow_ctrl.breathing_cycle_ms = 3000;  // Ĭ��3��һ������
    rainbow_ctrl.min_brightness = 5;  // ��С����Ϊ5%
    
    rt_kprintf("�ʺ�Ч���ѳ�ʼ��\n");
}

/* �ʺ�Ч���߳� */

static void rainbow_thread_entry(void* parameter)
{
    ws2812_hsv_t hsv;
    hsv.s = 1.0f;
    float hue = 0.0f;
    float angle = 0.0f;  // ����Ч���ĽǶ�
        while(1) {
        // ����Ƿ����ú��Ч��
        if (rainBowType == 1) {
            //��HSVɫ���У�0������ɫ��30���ǳ�ɫ��60���ǻ�ɫ��120������ɫ��180������ɫ��240������ɫ��300������ɫ��360���ֻص���ɫ��
            // ��-��-��-��-��-��-��Ļƻ轥�䣬��ɫ����
            // �趨ɫ�෶Χ����(0��)����(30��)����(30��)����(50��)����(30��)����(30��)����(0��)
            static float dusk_offset = 0.0f;
            float dusk_hue_range[] = {0.0f, 13.0f, 20.0f, 25.0f, 20.0f, 13.0f, 0.0f}; // ��ɫ����ӿ�
            int dusk_steps = 6; // ������

            for (int i = 0; i < WS2812_LED_NUM; i++) {
                // ����ÿ��LED�ڽ�����е�λ��
                float t = ((float)i / WS2812_LED_NUM + dusk_offset);
                if (t > 1.0f) t -= 1.0f;
                t *= dusk_steps; // ӳ�䵽����
                int idx = (int)t;
                float frac = t - idx; // �����ڲ�ֵ����

                // ���Բ�ֵɫ��
                float hue = dusk_hue_range[idx] + frac * (dusk_hue_range[idx + 1] - dusk_hue_range[idx]);
                ws2812_hsv_t hsv;
                hsv.h = hue;
                hsv.s = 1.0f;
                hsv.v = (float)rainbow_ctrl.brightness / 100.0f;

                ws2812_hsv_to_rgb(hsv,
                    &led_data[i * 3 + 0],
                    &led_data[i * 3 + 1],
                    &led_data[i * 3 + 2]);
            }
            dusk_offset += 0.006f; // ���������ٶ�
            if (dusk_offset > 1.0f) dusk_offset -= 1.0f;
 } 
        
        
        else if(rainBowType == 2){
            // ��-��-��-��-�������̽��䣬��ɫ����ɫ����ӿ�
            // �趨ɫ�෶Χ����(200��)����(170��)����(140��)����(170��)����(200��)
            static float ocean_offset = 0.0f;
            float ocean_hue_range[] = {240.0f, 170.0f, 130.0f, 170.0f, 240.0f}; // ɫ��ڵ�
            int ocean_steps = 4; // ������

            for (int i = 0; i < WS2812_LED_NUM; i++) {
                // ����ÿ��LED�ڽ�����е�λ��
                float t = ((float)i / WS2812_LED_NUM + ocean_offset);
                if (t > 1.0f) t -= 1.0f;
                t *= ocean_steps; // ӳ�䵽����
                int idx = (int)t;
                float frac = t - idx; // �����ڲ�ֵ����

                // ���Բ�ֵɫ��
                float hue = ocean_hue_range[idx] + frac * (ocean_hue_range[idx + 1] - ocean_hue_range[idx]);
                ws2812_hsv_t hsv;
                hsv.h = hue;
                hsv.s = 1.0f;
                hsv.v = (float)rainbow_ctrl.brightness / 100.0f;

                ws2812_hsv_to_rgb(hsv,
                    &led_data[i * 3 + 0],
                    &led_data[i * 3 + 1],
                    &led_data[i * 3 + 2]);
            }
            ocean_offset += 0.007f; // ���������ٶ�
            if (ocean_offset > 1.0f) ocean_offset -= 1.0f;
        }
        else if(rainBowType == 3){
            // AAAAA==3 ʱ������LED��ɫ����
            for (int i = 0; i < WS2812_LED_NUM; i++) {
                led_data[i * 3 + 0] = 200; // R
                led_data[i * 3 + 1] = 200; // G
                led_data[i * 3 + 2] = 200; // B
            }
        }
        else {
            float breathing_factor = 1.0f;
            
            if (rainbow_ctrl.breathing_enabled) {
                // ʹ�����Ҳ���������Ч��
                breathing_factor = (sinf(angle) + 1.0f) / 2.0f;  // ��-1��1ӳ�䵽0��1
                
                // �����������С���ȣ��򽫺�����Χ��Ϊ��С���ȵ��������
                if (rainbow_ctrl.min_brightness > 0) {
                    float min_factor = (float)rainbow_ctrl.min_brightness / 100.0f;
                    breathing_factor = min_factor + breathing_factor * (1.0f - min_factor);
                }
                
                // ���½Ƕ�
                angle += 2 * PI / (rainbow_ctrl.breathing_cycle_ms / rainbow_ctrl.delay_ms);
                if (angle >= 2 * PI) angle -= 2 * PI;
            }
            
            // ��������(0-100ת��Ϊ0.0-1.0)�������������
            hsv.v = (float)rainbow_ctrl.brightness / 100.0f * breathing_factor;
            
            // ��������LED��ɫ
            for(int i = 0; i < WS2812_LED_NUM; i++) {
                hsv.h = fmodf(hue + (float)i * (360.0f / WS2812_LED_NUM), 360.0f);
                
                ws2812_hsv_to_rgb(hsv, 
                    &led_data[i * 3 + 0],
                    &led_data[i * 3 + 1],
                    &led_data[i * 3 + 2]);
            }
            
            // ����ɫ��
            hue += rainbow_ctrl.hue_step;
            if(hue >= 360.0f) hue -= 360.0f;
        }
        
        // ����WS2812
        ws2812_update(led_data);

        // ���ݵ�ǰ�趨����ʱ������ʱ
        rt_thread_mdelay(rainbow_ctrl.delay_ms);
    }

}

/* �����ʺ�Ч�� */
void rainbow_start(uint8_t speed_level)
{    
    // ����ʺ��߳��Ѵ��ڣ���ɾ����
    if (rainbow_thread != RT_NULL)
    {
        rt_thread_delete(rainbow_thread);
        rainbow_thread = RT_NULL;
    }
    
    if(speed_level < 1) speed_level = 1;
    if(speed_level > 5) speed_level = 5;
    
    rainbow_ctrl.hue_step = 2.0f * speed_level;
    rainbow_ctrl.delay_ms = 50 / speed_level;
    
    // �����ʺ�Ч���߳�
    rainbow_thread = rt_thread_create("rainbow", 
                                      rainbow_thread_entry, 
                                      RT_NULL, 
                                      512, 
                                      9, 
                                      10);
    if(rainbow_thread != RT_NULL) {
        rt_thread_startup(rainbow_thread);
        rt_kprintf("�ʺ�Ч�����������ٶȼ���=%d\n", speed_level);
    } else {
        rt_kprintf("�ʺ�Ч���̴߳���ʧ�ܣ�\n");
    }
}

/* ֹͣ�ʺ�Ч�� */
void rainbow_stop(void)
{
    if (rainbow_thread != RT_NULL)
    {
        rt_thread_delete(rainbow_thread);
        rainbow_thread = RT_NULL;
        
        // �ر�����LED
        for (int i = 0; i < WS2812_LED_NUM * 3; i++)
        {
            led_data[i] = 0;
        }
        ws2812_update(led_data);
        
        rt_kprintf("�ʺ�ģʽ��ֹͣ\n");
    }
}

/* ���òʺ�Ч���ٶ� */
void rainbow_set_speed(uint8_t speed_level)
{
    if(speed_level < 1) speed_level = 1;
    if(speed_level > 5) speed_level = 5;
    
    rainbow_ctrl.hue_step = 2.0f * speed_level;
    rainbow_ctrl.delay_ms = 50 / speed_level;
    
    rt_kprintf("�ʺ�Ч���ٶ�������Ϊ����%d\n", speed_level);
}

/* ���òʺ�Ч������ */
void rainbow_set_brightness(uint8_t brightness)
{
    if(brightness > 100) brightness = 100;
    rainbow_ctrl.brightness = brightness;
    
    rt_kprintf("�ʺ�Ч������������Ϊ%d%%\n", brightness);
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


/* ����������Ч���Ĳʺ�Ч�� */
void rainbow_breathing_start(uint8_t speed_level, uint32_t breathing_cycle_s)
{
    // ֹͣ����ģʽ
    rainbow_stop();
    
    // ���ú�������
    if (breathing_cycle_s < 1) breathing_cycle_s = 1;
    if (breathing_cycle_s > 60) breathing_cycle_s = 60;
    rainbow_ctrl.breathing_cycle_ms = breathing_cycle_s * 1000;
    
    // ��������Ч��
    rainbow_ctrl.breathing_enabled = 1;
    
    // �����ʺ�Ч��
    rainbow_start(speed_level);
    
    rt_kprintf("�ʺ����Ч�����������ٶȼ���=%d����������=%d��\n", 
               speed_level, breathing_cycle_s);
}

/* ���òʺ����Ч������С���� */
void rainbow_set_min_brightness(uint8_t min_brightness)
{
    if (min_brightness >= rainbow_ctrl.brightness) {
        min_brightness = rainbow_ctrl.brightness / 2;  // ��ֹ��С���ȴ��ڵ����������
    }
    
    rainbow_ctrl.min_brightness = min_brightness;
    rt_kprintf("�ʺ����Ч����С����������Ϊ%d%%\n", min_brightness);
}

/* MSH��������ʺ����Ч�� */
static int cmd_rainbow_breathing(int argc, char **argv)
{
    int speed = 3;  // Ĭ���ٶȼ���
    int cycle = 3;  // Ĭ��3���������
    
    if (argc > 1) {
        speed = str_to_int(argv[1]);
    }
    
    if (argc > 2) {
        cycle = str_to_int(argv[2]);
    }
    
    rainbow_breathing_start(speed, cycle);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_breathing, start rainbow breathing effect [speed 1-5] [cycle_seconds]);

/* MSH������òʺ����Ч����С���� */
static int cmd_rainbow_min_brightness(int argc, char **argv)
{
    int min_brightness = 10;  // Ĭ����С����10%
    
    if (argc > 1) {
        min_brightness = str_to_int(argv[1]);
        if (min_brightness < 0) min_brightness = 0;
        if (min_brightness > 50) min_brightness = 50;  // ��󲻳���50%
    }
    
    rainbow_set_min_brightness(min_brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_rainbow_min_brightness, set rainbow breathing min brightness [0-50]);