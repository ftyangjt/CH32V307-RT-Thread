#include "ws2812b/rainbow.h"
#include "ws2812b/ws2812.h"
#include <rtthread.h>
#include <stdint.h>
#include "Cardinal.h"
#include <math.h>
#include <stdlib.h> // 添加stdlib.h用于atoi


static rt_thread_t breathing_thread = RT_NULL;

static struct {
    uint8_t r;
    uint8_t g; 
    uint8_t b;
    uint8_t max_brightness;  // 最大亮度百分比(0-100)
    uint32_t cycle_time_ms;  // 呼吸周期时间(毫秒)
} breathing_config = {255, 255, 255, 100, 3000};  // 默认白色，100%最大亮度，3秒周期
;
extern struct rt_semaphore *g_breathing_sem;
extern struct rt_semaphore *g_breathing_done_sem;
/* 呼吸灯效果线程 */
static void breathing_thread_entry()
{
    
    uint8_t color_data[WS2812_LED_NUM * 3];
    //使用正弦波计算当前亮度百分比(0-max_brightness)
    float brightness_factor = 0.0f;
    uint8_t current_brightness = 0;
    uint8_t r, g, b;
    
    while (1) {
        //永远等待主控线程信号量
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
        float step = 2 * PI / (breathing_config.cycle_time_ms / 20.0f); // 每20ms更新一次，计算步长

        rt_kprintf("灯光线程启动\n");
        while (run_time < 1000 * 10) { // 运行5秒
            // 使用正弦波计算当前亮度百分比(0-max_brightness)
            brightness_factor = (sinf(angle) + 1.0f) / 2.0f;  // 将-1到1的正弦值映射到0到1
            current_brightness = (uint8_t)(breathing_config.max_brightness * brightness_factor);
            
            // 根据亮度调整RGB值
            r = (uint8_t)((breathing_config.r * current_brightness) / 100);
            g = (uint8_t)((breathing_config.g * current_brightness) / 100);
            b = (uint8_t)((breathing_config.b * current_brightness) / 100);
            
            // 填充所有LED
            for (int i = 0; i < WS2812_LED_NUM; i++) {
                color_data[i * 3 + 0] = r; // R
                color_data[i * 3 + 1] = g; // G
                color_data[i * 3 + 2] = b; // B
            }
            
            
            ws2812_update(color_data);
            
            // 更新角度
            angle += step;
            if (angle >= 2 * PI) {
                angle -= 2 * PI;
            }
            
            rt_thread_mdelay(20); // 20ms刷新一次
            run_time += 20;
        }

        // 关闭所有LED
        for (int i = 0; i < WS2812_LED_NUM * 3; i++) color_data[i] = 0;
        ws2812_update(color_data);

        // 通知主控线程灯光已完成
        rainbow_start(1);
        rt_sem_release(g_breathing_done_sem);
    }
}

/* 启动呼吸灯效果 */
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
        rt_kprintf("呼吸灯效果已启动\n");
    } else {
        rt_kprintf("呼吸灯线程创建失败！\n");
    }
}
















/* 设置呼吸灯颜色和最大亮度 */
void breathing_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t max_brightness)
{
    breathing_config.r = r;
    breathing_config.g = g;
    breathing_config.b = b;
    
    if (max_brightness > 100) max_brightness = 100;
    breathing_config.max_brightness = max_brightness;
    
    rt_kprintf("呼吸灯颜色已设置为RGB:(%d,%d,%d)，最大亮度:%d%%\n", r, g, b, max_brightness);
}

/* MSH命令：启动呼吸灯效果 */
static int cmd_breathing(int argc, char **argv)
{
    int cycle_time = 3; // 默认3秒周期
    
    if (argc > 1) {
        cycle_time = str_to_int(argv[1]);
    }
    
    breathing_start(cycle_time);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_breathing, start breathing effect [cycle_time_seconds]);


/* MSH命令：设置呼吸灯颜色和亮度 */
static int cmd_breathing_color(int argc, char **argv)
{
    if (argc < 4) {
        rt_kprintf("用法: breathing_color r g b [max_brightness]\n");
        rt_kprintf("示例: breathing_color 255 0 0 80  设置为红色，最大亮度80%%\n");
        return -RT_ERROR;
    }
    
    int r = str_to_int(argv[1]);
    int g = str_to_int(argv[2]);
    int b = str_to_int(argv[3]);
    
    // 检查RGB范围
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    
    int max_brightness = 100; // 默认最大亮度
    if (argc > 4) {
        max_brightness = str_to_int(argv[4]);
        if (max_brightness < 0) max_brightness = 0;
        if (max_brightness > 100) max_brightness = 100;
    }
    
    breathing_set_color(r, g, b, max_brightness);
    return RT_EOK;
}
MSH_CMD_EXPORT(cmd_breathing_color, set breathing color: r g b [max_brightness]);

