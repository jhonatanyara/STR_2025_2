#pragma once
#include <stdbool.h>
#include "driver/ledc.h"
#include "hal/gpio_types.h"

// --- Cambia estos defines si usas otro pin o frecuencia ---
#define LED_PWM_GPIO        GPIO_NUM_18        // pin del LED verde
#define LED_PWM_ACTIVE_HIGH 1                  // 1: LED a GND; 0: LED a 3V3 (invertido)
#define LED_PWM_SPEEDMODE   LEDC_LOW_SPEED_MODE
#define LED_PWM_TIMER       LEDC_TIMER_0
#define LED_PWM_CHANNEL     LEDC_CHANNEL_0
#define LED_PWM_FREQ_HZ     5000               // 5 kHz
#define LED_PWM_RES         LEDC_TIMER_13_BIT  // resolución del timer
#define LED_PWM_MAX_DUTY    ((1U << LED_PWM_RES) - 1)

bool led_pwm_init(void);
void led_pwm_set_pct(int pct);   // 0..100 %
