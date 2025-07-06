#ifndef __CARDINAL_H__
#define __CARDINAL_H__

#include <rtthread.h>

// 全局时间变量（24小时制，精确到分钟）
typedef struct {
    int hour;
    int minute;
} cardinal_time_t;

extern cardinal_time_t g_cardinal_time;

// 定时任务结构体
typedef struct {
    float amount;           // 任务参数（支持小数）
    int light_mode;         // 灯光模式
    int water_duration;     // 水流时间
} cardinal_task_t;

// 任务表，索引为小时（0~23）
extern cardinal_task_t g_cardinal_tasks[24];

// Cardinal模块初始化
void cardinal_module_init(void);

// 由WIFI调用，校准时间
void cardinal_set_time(int hour, int minute);

// 由WIFI调用，更新定时任务表
void cardinal_update_tasks(const char *json);

// 导出主控线程信号量和舵机线程信号量
extern struct rt_semaphore *g_cardinal_main_sem;
extern struct rt_semaphore *g_cardinal_servo_sem;

// 新增：导出灯光线程信号量
extern struct rt_semaphore *g_breathing_sem;
extern struct rt_semaphore *g_breathing_done_sem;

// 新增：导出泵线程信号量
extern struct rt_semaphore *g_pump_sem;
extern struct rt_semaphore *g_pump_done_sem;

#endif // __CARDINAL_H__
