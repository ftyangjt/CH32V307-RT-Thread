#ifndef __DHT11_H__
#define __DHT11_H__

#include <stdint.h>

int dht11_read_data(uint8_t *temperature, uint8_t *humidity);

#endif // __DHT11_H__
