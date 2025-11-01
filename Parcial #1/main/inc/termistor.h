#ifndef TERMISTOR_H_
#define TERMISTOR_H_

#include "esp_adc/adc_oneshot.h"

// Fija la resistencia de referencia (R2) en ohmios
#define TERMISTOR_R2_OHMS 10000.0f

// Función para inicializar el canal ADC del termistor.
// Ahora acepta el handle directamente, no un puntero a un handle.
void termistor_init(adc_oneshot_unit_handle_t adc_handle);// inicializa el canal ADC para el termistor

// Función para leer la temperatura en grados Celsius
float termistor_get_temperature_celsius(void);// lee la temperatura en grados Celsius

#endif /* TERMISTOR_H_ */