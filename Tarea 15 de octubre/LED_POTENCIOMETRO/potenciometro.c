#include "inc/potenciometro.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"

static adc_oneshot_unit_handle_t s_adc1;
static adc_cali_handle_t         s_cali;
static bool                      s_tiene_cali = false;

bool potenciometro_init(void)
{
    // --- Unidad ---
    adc_oneshot_unit_init_cfg_t ucfg = { .unit_id = POT_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&ucfg, &s_adc1));

    // --- Canal ---
    adc_oneshot_chan_cfg_t ccf = {
        .atten    = POT_ADC_ATTEN,
        .bitwidth = POT_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1, POT_ADC_CHANNEL, &ccf));

    // --- Calibración (si está soportada) ---
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cal = {
        .unit_id  = POT_ADC_UNIT,
        .atten    = POT_ADC_ATTEN,
        .bitwidth = POT_ADC_BITWIDTH,
    };
    if (adc_cali_create_scheme_curve_fitting(&cal, &s_cali) == ESP_OK) {
        s_tiene_cali = true;
    } else {
        s_tiene_cali = false;
    }
#else
    s_tiene_cali = false;
#endif
    return true;
}

int potenciometro_leer_crudo(void)
{
    int valor = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc1, POT_ADC_CHANNEL, &valor));
    return valor;
}

int potenciometro_leer_crudo_prom(int n_muestras)
{
    if (n_muestras < 1) n_muestras = 1;
    int suma = 0;
    for (int i = 0; i < n_muestras; ++i) {
        int v = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc1, POT_ADC_CHANNEL, &v));
        suma += v;
    }
    return suma / n_muestras;
}

int potenciometro_leer_milivoltios(void)
{
    int crudo = potenciometro_leer_crudo();

    int mv = 0;
    if (s_tiene_cali) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali, crudo, &mv));
    } else {
        // Aproximación simple: 12 bits y atenuación 12dB ~ 3.3V full-scale
        const int FS_COUNTS = 4095;
        mv = (int)((crudo * 3300.0f) / (float)FS_COUNTS);
    }
    return mv;
}

int potenciometro_porcentaje_0_100(void)
{
    // 0..~3300 mV -> 0..100 %
    int mv  = potenciometro_leer_milivoltios();
    int pct = (int)((mv * 100.0f) / 3300.0f);

    if (pct < 0) pct = 0;
    else if (pct > 100) pct = 100;

    return pct;
}

