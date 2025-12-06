#include "led_pwm.h"
#include "esp_err.h"

static inline uint32_t duty_from_pct(int pct)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    uint32_t duty = (uint32_t)((pct / 100.0f) * LED_PWM_MAX_DUTY);

#if LED_PWM_ACTIVE_HIGH
    return duty;                         // LED a GND: duty directo
#else
    return LED_PWM_MAX_DUTY - duty;      // LED a 3V3: invertir
#endif
}

bool led_pwm_init(void)
{
    ledc_timer_config_t tcfg = {
        .speed_mode       = LED_PWM_SPEEDMODE,
        .duty_resolution  = LED_PWM_RES,
        .timer_num        = LED_PWM_TIMER,
        .freq_hz          = LED_PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    ledc_channel_config_t ccfg = {
        .gpio_num   = LED_PWM_GPIO,
        .speed_mode = LED_PWM_SPEEDMODE,
        .channel    = LED_PWM_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LED_PWM_TIMER,
        .duty       = duty_from_pct(0),
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
    return true;
}

void led_pwm_set_pct(int pct)
{
    uint32_t duty = duty_from_pct(pct);
    ESP_ERROR_CHECK(ledc_set_duty(LED_PWM_SPEEDMODE, LED_PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LED_PWM_SPEEDMODE, LED_PWM_CHANNEL));
}
