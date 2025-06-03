#ifndef __WIFI_H__
#define __WIFI_H__

#include <rtthread.h>
#include <rtdevice.h>

typedef struct {
    float interval;
    float amount;
    int blink;
} wifi_param_t;

extern wifi_param_t g_wifi_param;

void wifi_module_init(void);
void wifi_param_save(void);
void wifi_param_load(void);
void wifi_power_ctrl(rt_bool_t on);

#endif // __WIFI_H__
