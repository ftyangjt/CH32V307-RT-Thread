#ifndef __ATK_H__
#define __ATK_H__

#include <rtthread.h>
#include <rtdevice.h>

void atk_uart_init(void);
void send_at_cmd(const char *cmd);
void atk8266_wifi_ap_web_init(void);

#endif // __ATK_H__
