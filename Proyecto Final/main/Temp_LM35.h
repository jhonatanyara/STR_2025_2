#ifndef TEMP_LM35_H
#define TEMP_LM35_H

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// En ESP32, GPIO 35 es ADC1 Canal 6
#define LM35_ADC_CHANNEL ADC_CHANNEL_7 

void temp_sensor_init(void);
float temp_sensor_read_celsius(void);

#endif