#ifndef __PUMP_H__
#define __PUMP_H__

#include <rtdevice.h>
#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>

//直接使用引脚编号
#define RELAY_PIN    87

void pump_on(void);
void pump_off(void);
void submersible_pump_init(void);
void pump_thread_init(void);

#endif /* __PUMP_H__ */