#ifndef POTENCIOMETRO_H
#define POTENCIOMETRO_H

#include "esp_adc/adc_oneshot.h"

/**
 * @brief Inicializa el canal ADC para el potenciómetro.
 *
 * @param adc_handle Handle de la unidad ADC (e.g., ADC_UNIT_1).
 */
void potenciometro_init(adc_oneshot_unit_handle_t adc_handle);

/**
 * @brief Lee el valor del potenciómetro y lo normaliza entre 0.0 y 1.0.
 *
 * @return Valor flotante entre 0.0 y 1.0.
 */
float potenciometro_get_normalized_value(void);

#endif // POTENCIOMETRO_H
