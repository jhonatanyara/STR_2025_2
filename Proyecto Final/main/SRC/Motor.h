#ifndef MOTOR_H
#define MOTOR_H

#include "driver/ledc.h"
#include "driver/gpio.h"

// --- Configuración de Hardware del Motor ---
#define FAN_PIN GPIO_NUM_23              // Pin PWM para el ventilador (Corregido: GPIO 23)
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT  // Resolución de 13 bits (0 a 8191)
#define LEDC_FREQUENCY 5000              // Frecuencia de PWM de 5 kHz

// --- Funciones del Motor ---

/**
 * @brief Inicializa el temporizador y el canal PWM para el motor.
 */
void motor_init(void);

/**
 * @brief Establece la velocidad del ventilador en porcentaje.
 * * @param percent Velocidad deseada (0 a 100).
 */
void motor_set_speed_percent(int percent);

#endif // MOTOR_H