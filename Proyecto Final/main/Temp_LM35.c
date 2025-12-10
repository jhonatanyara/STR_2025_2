#include "Temp_LM35.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LM35";

// --- CONFIGURACIÓN ---
// Usamos ADC_ATTEN_DB_12 (Rango 0-3.3V) para seguridad y estabilidad
#define LM35_ATTEN ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;
static bool adc_initialized = false;
static bool calibrated = false;

// Variable para el filtro de suavizado
static float smoothed_temp = -1.0; 
// Factor de suavizado (0.1 = Lento y estable, 0.5 = Rápido)
#define FILTER_ALPHA 0.10f 

// --- FUNCIÓN QUE CARGA LA CALIBRACIÓN DE FÁBRICA ---
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool cal_success = false;

    // Intenta calibración por Curva (Mejor precisión)
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!cal_success) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit, .atten = atten, .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) cal_success = true;
    }
#endif

    // Si no, intenta calibración Lineal
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!cal_success) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit, .atten = atten, .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) cal_success = true;
    }
#endif

    *out_handle = handle;
    if (cal_success) {
        ESP_LOGI(TAG, "Calibración de fábrica cargada correctamente");
    } else {
        ESP_LOGW(TAG, "No se encontró calibración, usando modo crudo");
    }
    return cal_success;
}

void temp_sensor_init(void) {
    if (adc_initialized) return;

    // 1. Iniciar Unidad ADC
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Configurar Canal
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = LM35_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LM35_ADC_CHANNEL, &config));

    // 3. Cargar datos de calibración del chip
    calibrated = adc_calibration_init(ADC_UNIT_1, LM35_ATTEN, &adc1_cali_handle);

    adc_initialized = true;
}

float temp_sensor_read_celsius(void) {
    if (!adc_initialized) temp_sensor_init();

    int raw_val;
    int voltage_mv = 0;
    long sum_raw = 0;
    int samples = 64; 

    // 1. Tomar muchas muestras
    for (int i = 0; i < samples; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LM35_ADC_CHANNEL, &raw_val));
        sum_raw += raw_val;
        // Espera un poco más larga para que el capacitor se cargue bien
        esp_rom_delay_us(60); 
    }
    int avg_raw = sum_raw / samples;

    // 2. CONVERTIR A VOLTAJE (Milivoltios)
    if (calibrated) {
        // Esta función corrige la curva y debería arreglar el problema de los 5 grados
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, avg_raw, &voltage_mv));
    } else {
        // Fallback manual solo si falla la calibración
        voltage_mv = avg_raw * 3300 / 4095;
    }

    // 3. CONVERTIR A GRADOS
    float current_temp = (float)voltage_mv / 10.0f;

    // 4. OFFSET MANUAL DE EMERGENCIA (Si sigue bajo, descomenta la línea de abajo)
    // current_temp += 3.0; // Sumar 3 grados si ves que siempre le falta un poco

    // 5. FILTRO DE SUAVIZADO
    if (smoothed_temp < 0) {
        smoothed_temp = current_temp;
    } else {
        smoothed_temp = (current_temp * FILTER_ALPHA) + (smoothed_temp * (1.0f - FILTER_ALPHA));
    }

    return smoothed_temp;
}