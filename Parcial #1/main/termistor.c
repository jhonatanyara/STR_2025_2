#include "termistor.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include <math.h> // Necesario para la función log() y pow()

static const char *TERM_TAG = "TERMISTOR";

// Variables estáticas para almacenar el handle del ADC y el canal configurado.
static adc_oneshot_unit_handle_t global_adc_handle = NULL;
static adc_channel_t termistor_channel = ADC_CHANNEL_9;

// --- CONFIGURACIÓN DEL HARDWARE ---
// Termistor conectado al GPIO34 (ADC1 Channel 6)
#define TERMISTOR_ADC_CHANNEL   ADC_CHANNEL_6 
// Voltaje de referencia usado por el ADC (3.3V es el comun en ESP32)
#define V_REF                   3.3f 
// Resolucion maxima del ADC (12 bits)
#define ADC_MAX_VAL             4095.0f 
// Resistor fijo en el divisor de voltaje (comunmente 10k Ohm)
#define SERIES_RESISTOR         10000.0f 

// --- CONSTANTES DEL TERMISTOR (NTC 10K 3950) ---
#define THERMISTOR_NOMINAL      10000.0f    // Resistencia a la temperatura nominal (Ohms)
#define TEMPERATURE_NOMINAL     25.0f       // Temperatura nominal (Celsius)
#define B_COEFFICIENT           3950.0f     // Coeficiente Beta (Kelvin)

// Función de inicialización
void termistor_init(adc_oneshot_unit_handle_t adc_handle) {
    global_adc_handle = adc_handle;
    termistor_channel = TERMISTOR_ADC_CHANNEL; // Asignamos el canal

    // 1. Configuración del canal ADC para el termistor
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11, // Atenuacion de 11 dB (rango de 0 a 3.3V)
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    // Configurar el canal
    ESP_ERROR_CHECK(adc_oneshot_config_channel(global_adc_handle, termistor_channel, &config));
    ESP_LOGI(TERM_TAG, "Termistor en canal %d inicializado con atenuación DB_11.", termistor_channel);
}

// Función para obtener la temperatura en Celsius
float termistor_get_temperature_celsius(void) {
    if (global_adc_handle == NULL) {
        ESP_LOGE(TERM_TAG, "ADC handle no inicializado. Devuelve 0.0");
        return 0.0f;
    }

    int raw_val;
    // Leer el valor crudo del ADC
    esp_err_t ret = adc_oneshot_read(global_adc_handle, termistor_channel, &raw_val);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TERM_TAG, "Error al leer ADC: %s", esp_err_to_name(ret));
        return 0.0f;
    }

    // 1. Calcular la Resistencia del Termistor (R_th)
    // Asumimos un divisor de voltaje Pull-Up (R_serie a VCC, Termistor a GND, punto medio al ADC)
    // Fórmula del divisor: V_ADC = V_REF * (R_th / (R_serie + R_th))
    // Despejando R_th: R_th = R_serie * (V_REF / V_ADC - 1)
    
    // Convertir el valor crudo a voltaje (aunque no es estrictamente necesario, ayuda a la comprension)
    // float v_adc = (float)raw_val * V_REF / ADC_MAX_VAL; 

    // Simplificando la formula usando valores ADC:
    // R_th = R_serie * (ADC_MAX_VAL / raw_val - 1)
    float resistance = SERIES_RESISTOR * ((ADC_MAX_VAL / (float)raw_val) - 1.0f);

    // 2. Usar la ecuación de Steinhart-Hart simplificada (Modelo Beta)
    
    float steinhart;
    
    // Calcular el logaritmo natural de la razón R_th / R_nominal
    steinhart = resistance / THERMISTOR_NOMINAL; // (R/R0)
    steinhart = log(steinhart);                  // ln(R/R0)
    
    // Sumar el inverso de la Temperatura Nominal (en Kelvin)
    steinhart /= B_COEFFICIENT;                  // (1/B) * ln(R/R0)
    steinhart += 1.0f / (TEMPERATURE_NOMINAL + 273.15f); // + (1/T0)
    
    // Calcular el inverso (T_kelvin)
    steinhart = 1.0f / steinhart;                // T_kelvin
    
    // 3. Convertir a Celsius
    float temp_celsius = steinhart - 273.15f;    // T_celsius

    // Si la temperatura medida baja al calentar el termistor, significa que la formula
    // o la configuración del divisor (Pull-Up vs Pull-Down) estan invertidas.
    // Si usaste Pull-Down (Termistor a VCC, R_serie a GND, ADC al punto medio), la formula es:
    // R_th = R_serie * V_ADC / (V_REF - V_ADC) 
    // Que es equivalente a: R_th = R_serie * raw_val / (ADC_MAX_VAL - raw_val)

    // Si la fórmula Pull-Up invertida funciona, es porque el hardware está en Pull-Down:
    // if (temp_celsius < 0) {
    //     // Si los valores calculados son negativos o anómalos, probar la configuracion invertida
    // }

    return temp_celsius;
}
