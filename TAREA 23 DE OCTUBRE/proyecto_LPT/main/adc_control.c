#include "adc_control.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *ADC_TAG = "ADC_CONTROL";

bool adc_control_init(adc_oneshot_unit_handle_t *adc_handle) {
    // Configuracion de la unidad ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1, // Usar la Unidad 1 del ADC
        // .clk_src = ADC_CLK_SRC_DEFAULT, // <--- ELIMINADO: Causa error en IDF v5.5.1
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    // Crear la nueva unidad ADC1. Esto solo debe llamarse una vez.
    esp_err_t ret = adc_oneshot_new_unit(&init_config, adc_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(ADC_TAG, "Error al inicializar ADC1: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(ADC_TAG, "Unidad ADC1 inicializada correctamente.");
    return true;
}
