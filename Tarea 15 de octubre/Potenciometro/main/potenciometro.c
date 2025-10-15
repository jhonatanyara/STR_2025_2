#include "potenciometro.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"

static adc_oneshot_unit_handle_t s_adc1 = NULL;
static adc_cali_handle_t         s_cali = NULL;
static bool                      s_tiene_cali = false;

bool potenciometro_init(void)
{
    // 1) Crear la unidad ADC
    adc_oneshot_unit_init_cfg_t ucfg = {
        .unit_id = POT_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&ucfg, &s_adc1));

    // 2) Configurar el canal (aten = 11dB para ~0..3.3V, 12 bits)
    adc_oneshot_chan_cfg_t ccfg = {
        .atten    = POT_ADC_ATTEN,
        .bitwidth = POT_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1, POT_ADC_CHANNEL, &ccfg));

    // 3) Intentar calibracion por "curve fitting" (si el chip la soporta)
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
    int raw = 0;
    if (s_adc1) {
        adc_oneshot_read(s_adc1, POT_ADC_CHANNEL, &raw);
    }
    return raw;
}

int potenciometro_leer_crudo_prom(int n)
{
    if (n <= 1) return potenciometro_leer_crudo();
    int64_t acum = 0;
    for (int i = 0; i < n; ++i) {
        acum += potenciometro_leer_crudo();
    }
    return (int)(acum / n);
}

int potenciometro_leer_milivoltios(void)
{
    int raw = potenciometro_leer_crudo_prom(8);
    int mv  = 0;

    if (s_tiene_cali && s_cali) {
        if (adc_cali_raw_to_voltage(s_cali, raw, &mv) != ESP_OK) {
            mv = 0;
        }
    } else {
        // Aproximación sin calibración (12 bits, 0..4095 -> 0..3300mV)
        mv = (int)( (raw * 3300.0f) / 4095.0f );
    }
    return mv;
}

int potenciometro_porcentaje_0_100(void)
{
    // Porcentaje por recuento crudo
    int raw = potenciometro_leer_crudo_prom(8);
    int pct = (int)( (raw * 100.0f) / 4095.0f );
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}
