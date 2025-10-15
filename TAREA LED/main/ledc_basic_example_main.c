#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ====== CONFIGURACIÓN DE HARDWARE ======
#define RGB_RED_GPIO     25
#define RGB_GREEN_GPIO   26
#define RGB_BLUE_GPIO    27

// Pon 1 si tu LED es de ÁNODO COMÚN (se enciende con nivel bajo)
#define RGB_COMMON_ANODE 0

// ====== CONFIGURACIÓN LEDC ======
#define LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_TIMER        LEDC_TIMER_0
#define LEDC_FREQ_HZ      (4000)
#define LEDC_DUTY_RES     LEDC_TIMER_13_BIT          // 13 bits -> 0..8191
#define LEDC_MAX_DUTY     ((1 << LEDC_DUTY_RES) - 1) // 8191

// Canales asignados a cada color
#define CH_RED    LEDC_CHANNEL_0
#define CH_GREEN  LEDC_CHANNEL_1
#define CH_BLUE   LEDC_CHANNEL_2

static inline uint32_t rgb_8bit_to_duty(uint8_t x)
{
    // Mapea 0..255 a 0..LEDC_MAX_DUTY (sin gamma, directo)
    uint32_t duty = (uint32_t)x * LEDC_MAX_DUTY / 255;
#if RGB_COMMON_ANODE
    // En ánodo común el “encendido” es duty bajo (invertimos)
    duty = LEDC_MAX_DUTY - duty;
#endif
    return duty;
}

static esp_err_t ledc_rgb_init(void)
{
    // Timer
    const ledc_timer_config_t tcfg = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    // Canal ROJO
    const ledc_channel_config_t ch_r = {
        .gpio_num       = RGB_RED_GPIO,
        .speed_mode     = LEDC_MODE,
        .channel        = CH_RED,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = rgb_8bit_to_duty(0), // arranca apagado
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_r));

    // Canal VERDE
    const ledc_channel_config_t ch_g = {
        .gpio_num       = RGB_GREEN_GPIO,
        .speed_mode     = LEDC_MODE,
        .channel        = CH_GREEN,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = rgb_8bit_to_duty(0),
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_g));

    // Canal AZUL
    const ledc_channel_config_t ch_b = {
        .gpio_num       = RGB_BLUE_GPIO,
        .speed_mode     = LEDC_MODE,
        .channel        = CH_BLUE,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = rgb_8bit_to_duty(0),
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_b));

    return ESP_OK;
}

static void rgb_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, CH_RED,   rgb_8bit_to_duty(r)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, CH_RED));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, CH_GREEN, rgb_8bit_to_duty(g)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, CH_GREEN));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, CH_BLUE,  rgb_8bit_to_duty(b)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, CH_BLUE));
}

void app_main(void)
{
    ESP_ERROR_CHECK(ledc_rgb_init());

    // Demo: rotación de colores básicos y algunos mixtos
    while (1) {
        rgb_set_color(255, 0, 0);   vTaskDelay(pdMS_TO_TICKS(800)); // Rojo
        rgb_set_color(0, 255, 0);   vTaskDelay(pdMS_TO_TICKS(800)); // Verde
        rgb_set_color(0, 0, 255);   vTaskDelay(pdMS_TO_TICKS(800)); // Azul
        rgb_set_color(255, 255, 255); vTaskDelay(pdMS_TO_TICKS(800)); // Blanco
        rgb_set_color(0, 0, 0);     vTaskDelay(pdMS_TO_TICKS(800)); // Apagado
    }
}