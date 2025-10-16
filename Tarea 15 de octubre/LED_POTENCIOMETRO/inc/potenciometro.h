#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==== Config por defecto (puedes cambiar el canal si usas otro pin) ====
// ADC1_CH6  -> GPIO34 (entrada solamente)
#define POT_ADC_UNIT        ADC_UNIT_1
#define POT_ADC_CHANNEL     ADC_CHANNEL_6       // GPIO34 en ESP32
#define POT_ADC_ATTEN       ADC_ATTEN_DB_12     // ~0..3.3V (DB_11 está deprecado)
#define POT_ADC_BITWIDTH    ADC_BITWIDTH_DEFAULT

bool potenciometro_init(void);
int  potenciometro_leer_crudo(void);
int  potenciometro_leer_crudo_prom(int n_muestras);
int  potenciometro_leer_milivoltios(void);      // usa calibración si hay
int  potenciometro_porcentaje_0_100(void);      // 0..100 %

#ifdef __cplusplus
}
#endif
