#pragma once
#include <stdbool.h>
#include "driver/adc.h"

// ==== Configuración de hardware ====
// GPIO33 = ADC1_CH5 (puedes cambiarlo si lo necesitas)
#define NTC_ADC_CHANNEL   ADC_CHANNEL_5

// Parámetros del divisor y del NTC
#define NTC_VCC           3.3f
#define NTC_R_FIXED       1000.0f   // 10kΩ fijo
#define NTC_R0            10000.0f   // 10kΩ @25°C
#define NTC_T0_K          298.15f    // 25°C
#define NTC_BETA          3950.0f    // Beta típico

bool  termistor_init(void);          // ADC1 + canal + (opcional) calibración
int   termistor_read_raw(void);      // raw 0..4095 (promediado)
float termistor_read_millivolts(void);
float termistor_read_celsius(void);
