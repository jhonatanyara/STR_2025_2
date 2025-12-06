#include "potenciometro.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *POT_TAG = "POTENCIOMETRO";

// Variables estáticas para almacenar el handle del ADC y el canal configurado.
static adc_oneshot_unit_handle_t global_adc_handle = NULL;
// Usamos ADC_CHANNEL_MAX o 0, ya que el valor real se asigna en potenciometro_init.
static adc_channel_t pot_channel = ADC_CHANNEL_9; 

// --- Configuración del Canal ---
// Asumimos que el potenciómetro está conectado al GPIO35 (ADC1 Channel 7).
#define POT_ADC_CHANNEL ADC_CHANNEL_7 

// Función de inicialización (solo configura el canal, no inicializa la unidad)
void potenciometro_init(adc_oneshot_unit_handle_t adc_handle) {
    global_adc_handle = adc_handle;
    pot_channel = POT_ADC_CHANNEL; // Asignamos el canal

    // 1. Configuración del canal ADC para el potenciómetro
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12, // Atenuación de 12 dB (rango de 0 a 3.3V)
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    // Configurar el canal
    ESP_ERROR_CHECK(adc_oneshot_config_channel(global_adc_handle, pot_channel, &config));
    ESP_LOGI(POT_TAG, "Potenciómetro en canal %d inicializado con atenuación DB_12.", pot_channel);
}

// Función para leer el valor normalizado del potenciómetro (0.0 a 1.0)
float potenciometro_get_normalized_value(void) {
    if (global_adc_handle == NULL) {
        // En caso de que se llame antes de la inicialización
        ESP_LOGE(POT_TAG, "ADC handle no inicializado. Devuelve 0.0");
        return 0.0f;
    }

    int raw_val;
    // Leer el valor crudo del ADC
    esp_err_t ret = adc_oneshot_read(global_adc_handle, pot_channel, &raw_val);
    
    if (ret != ESP_OK) {
        ESP_LOGE(POT_TAG, "Error al leer ADC: %s", esp_err_to_name(ret));
        return 0.0f;
    }

    // Normalización: Dividir el valor crudo (máximo 4095 para 12 bits) por el máximo.
    // Usamos 4095.0f para asegurar el cálculo flotante.
    float normalized_value = (float)raw_val / 4095.0f;
    
    return normalized_value;
}