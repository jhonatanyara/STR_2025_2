#include <math.h>
#include <stdio.h>
#include "termistor.h"
#include "esp_adc/adc_oneshot.h"

/* Calibración: puede no estar disponible según eFuses/IDF/target.
   Protegemos con macros para compilar en todos los casos. */
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static adc_oneshot_unit_handle_t s_adc1 = NULL;
static adc_cali_handle_t         s_cali = NULL;
static bool                      s_has_cali = false;

static float raw_to_mv(int raw)
{
    int mv = 0;
#if defined(ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED)
    if (s_has_cali && adc_cali_raw_to_voltage(s_cali, raw, &mv) == ESP_OK) {
        return (float)mv;
    }
#endif
    // Aprox. 12 bits, 11 dB -> ~0..3300 mV
    return (raw / 4095.0f) * 3300.0f;
}

bool termistor_init(void)
{
    // Unidad y canal
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    if (adc_oneshot_new_unit(&unit_cfg, &s_adc1) != ESP_OK) return false;

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = ADC_ATTEN_DB_11,        // ~0..3.3V
        .bitwidth = ADC_BITWIDTH_DEFAULT // 12 bits
    };
    if (adc_oneshot_config_channel(s_adc1, NTC_ADC_CHANNEL, &ch_cfg) != ESP_OK)
        return false;

    // Calibración si el esquema está soportado
#if defined(ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED)
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali) == ESP_OK) {
        s_has_cali = true;
        printf("ADC calibrado (curve fitting)\n");
    } else {
        s_has_cali = false;
        printf("ADC sin calibración (aprox lineal)\n");
    }
#else
    s_has_cali = false;
    printf("ADC: esquema de calibración no soportado; usaré aproximación lineal\n");
#endif

    return true;
}

int termistor_read_raw(void)
{
    const int N = 8;
    int sum = 0, v = 0;
    for (int i = 0; i < N; ++i) {
        adc_oneshot_read(s_adc1, NTC_ADC_CHANNEL, &v);
        sum += v;
    }
    return sum / N;
}

float termistor_read_millivolts(void)
{
    return raw_to_mv(termistor_read_raw());
}

float termistor_read_celsius(void)
{
    float v = termistor_read_millivolts() / 1000.0f; // V nodo
    if (v <= 0.0f) v = 0.001f;
    if (v >= NTC_VCC) v = NTC_VCC - 0.001f;

    // Divisor: VCC—R_FIXED—nodo—NTC—GND
    float r_ntc = NTC_R_FIXED * (v / (NTC_VCC - v));
    float invT  = (1.0f/NTC_T0_K) + (1.0f/NTC_BETA) * logf(r_ntc / NTC_R0);
    return (1.0f/invT) - 273.15f;
}
