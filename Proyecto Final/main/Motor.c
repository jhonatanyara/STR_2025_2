#include "Motor.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

void motor_init(void) {
    // 1. Configuración del Temporizador LEDC (Time Base)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configuración del Canal LEDC (Channel)
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = FAN_PIN,
        .duty           = 0, // Inicia apagado
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(TAG, "Motor (PWM) inicializado en GPIO %d", FAN_PIN);
}

void motor_set_speed_percent(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    // Valor máximo para 13 bits es 8191 (2^13 - 1)
    const uint32_t LEDC_MAX_DUTY = (1 << LEDC_DUTY_RES) - 1;

    // Convertir porcentaje (0-100) a ciclo de trabajo (0-8191)
    uint32_t duty = (percent * LEDC_MAX_DUTY) / 100;

    // Aplicar nuevo ciclo de trabajo
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}


// el motor se conecta en el pin 35 del ESP32 y el otro pin a tierra y el pwm controla la velocidad del motor
// y esta es la libreria para controlar el motor con pwm