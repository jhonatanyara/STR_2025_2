#include "led_pwm.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "LED_PWM";

// --- Definiciones Comunes de PWM ---
#define LEDC_TIMER                  LEDC_TIMER_0
#define LEDC_MODE                   LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES               LEDC_TIMER_10_BIT // 10 bits de resolución (0 - 1023)
#define LEDC_FREQUENCY              (5000) // Frecuencia de 5 KHz

// --- Pines y Canales PWM para RGB ---
// CONEXIONES DE HARDWARE:
#define LED_RED_GPIO                (25) // Pin GPIO para el LED Rojo
#define LED_GREEN_GPIO              (26) // Pin GPIO para el LED Verde
#define LED_BLUE_GPIO               (27) // Pin GPIO para el LED Azul

// Canales PWM (cada color usa un canal diferente)
#define LEDC_CHANNEL_RED            LEDC_CHANNEL_0
#define LEDC_CHANNEL_GREEN          LEDC_CHANNEL_1
#define LEDC_CHANNEL_BLUE           LEDC_CHANNEL_2


/**
 * @brief Configura un canal PWM LEDC específico.
 * @param gpio_num El pin GPIO para el LED.
 * @param channel El canal LEDC a utilizar (0, 1, o 2).
 */
static void led_channel_config(int gpio_num, ledc_channel_t channel)
{
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = channel,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio_num,
        .duty           = 0, // Inicia con el LED apagado
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


// Función de inicialización declarada en led_pwm.h
void led_pwm_init(void)
{
    // 1. Configuración del Timer (Solo se necesita un timer para todos los canales)
    ledc_timer_config_t ledc_timer = {
        .speed_mode     = LEDC_MODE,
        .timer_num      = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz        = LEDC_FREQUENCY,
        .clk_cfg        = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configuración de los Canales PWM individuales
    led_channel_config(LED_RED_GPIO, LEDC_CHANNEL_RED);
    led_channel_config(LED_GREEN_GPIO, LEDC_CHANNEL_GREEN);
    led_channel_config(LED_BLUE_GPIO, LEDC_CHANNEL_BLUE);

    ESP_LOGI(TAG, "Sistema PWM RGB inicializado en canales 0, 1, 2 a %d Hz", LEDC_FREQUENCY);
    ESP_LOGI(TAG, "Pines: Rojo (GPIO %d), Verde (GPIO %d), Azul (GPIO %d)", LED_RED_GPIO, LED_GREEN_GPIO, LED_BLUE_GPIO);
}

// Función auxiliar para establecer el ciclo de trabajo en un canal específico
static void set_channel_duty(ledc_channel_t channel, uint32_t duty)
{
    // Asegura que el valor no exceda la resolución máxima (1023 para 10 bits)
    uint32_t max_duty = (1 << LEDC_DUTY_RES) - 1;
    if (duty > max_duty) {
        duty = max_duty;
    }

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, channel));
}


// --- Funciones de Control RGB (Llamadas desde main.c) ---

void led_pwm_set_red(uint32_t duty)
{
    set_channel_duty(LEDC_CHANNEL_RED, duty);
    ESP_LOGD(TAG, "Rojo: duty=%lu", duty);
}

void led_pwm_set_green(uint32_t duty)
{
    set_channel_duty(LEDC_CHANNEL_GREEN, duty);
    ESP_LOGD(TAG, "Verde: duty=%lu", duty);
}

void led_pwm_set_blue(uint32_t duty)
{
    set_channel_duty(LEDC_CHANNEL_BLUE, duty);
    ESP_LOGD(TAG, "Azul: duty=%lu", duty);
}

// Implementación de la función led_pwm_set_duty (mantenida por compatibilidad, aunque no se usa en main.c)
void led_pwm_set_duty(uint32_t duty)
{
    // Por simplicidad, esta función ya no hace nada o puede ser eliminada
    // si no hay un LED simple conectado al GPIO 2.
    // Si todavía tiene el LED simple en GPIO 2, debería configurarlo 
    // como otro canal aquí. Por ahora, la dejamos en blanco ya que el 
    // control lo hacen las funciones de color.
}
void led_pwm_set_rgb(uint32_t duty_r, uint32_t duty_g, uint32_t duty_b)
{
    set_channel_duty(LEDC_CHANNEL_RED, duty_r);
    set_channel_duty(LEDC_CHANNEL_GREEN, duty_g);
    set_channel_duty(LEDC_CHANNEL_BLUE, duty_b);
    ESP_LOGD(TAG, "RGB: R=%lu, G=%lu, B=%lu", duty_r, duty_g, duty_b);
}
