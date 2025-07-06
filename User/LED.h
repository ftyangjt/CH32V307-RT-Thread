#ifndef __LED_H__
#define __LED_H__

#include <rtthread.h>
#include <stdint.h>

void breathing_start();
void breathing_stop(void);
void breathing_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t max_brightness);

#endif // __LED_H__
void breathing_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t max_brightness);
