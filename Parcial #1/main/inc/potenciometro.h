#ifndef POTENCIOMETRO_H
#define POTENCIOMETRO_H

#include "esp_adc/adc_oneshot.h"

/**
 * @brief Inicializa el canal ADC para el potenciómetro.
 *
 * @param adc_handle Handle de la unidad ADC (e.g., ADC_UNIT_1).
 */
void potenciometro_init(adc_oneshot_unit_handle_t adc_handle);// inicializa el canal ADC para el potenciómetro

/**
 * @brief Lee el valor del potenciómetro y lo normaliza entre 0.0 y 1.0.
 *
 * @return Valor flotante entre 0.0 y 1.0.
 */
float potenciometro_get_normalized_value(void);// lee el valor del potenciómetro y lo normaliza entre 0.0 y 1.0

/**
 * @brief Lee el voltaje del potenciómetro (en voltios).
 *
 * Esta función convierte el valor normalizado (0.0 - 1.0) a voltios
 * asumiendo referencia de 3.3V y la atenuación configurada en el ADC.
 *
 * @return Voltaje aproximado en voltios (float). Si el ADC no está
 * inicializado, devuelve 0.0f.
 */
float potenciometro_get_voltage(void);// lee el voltaje del potenciómetro (en voltios)

#endif // POTENCIOMETRO_H
