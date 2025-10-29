#ifndef ADC_CONTROL_H
#define ADC_CONTROL_H

#include "esp_adc/adc_oneshot.h"
#include <stdbool.h>

/**
 * @brief Inicializa la unidad ADC1 una única vez.
 *
 * @param adc_handle Puntero para almacenar el handle de la unidad ADC inicializada.
 * @return true si la inicialización fue exitosa, false en caso contrario.
 */
bool adc_control_init(adc_oneshot_unit_handle_t *adc_handle);

#endif // ADC_CONTROL_H
