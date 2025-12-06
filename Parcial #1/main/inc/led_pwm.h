#ifndef LED_PWM_H
#define LED_PWM_H

#include <stdint.h>

/**
 * @brief Inicializa el driver LEDC (PWM) para los tres canales RGB.
 * * Configura el timer, los canales y las salidas PWM para el LED RGB.
 */
void led_pwm_init(void);// inicializa el driver LEDC para controlar el LED RGB

/**
 * @brief Establece el ciclo de trabajo (brillo) para los canales Rojo, Verde y Azul.
 * * @param duty_r Ciclo de trabajo para el canal Rojo (0-100%).
 * @param duty_g Ciclo de trabajo para el canal Verde (0-100%).
 * @param duty_b Ciclo de trabajo para el canal Azul (0-100%).
 */
void led_pwm_set_rgb(uint32_t duty_r, uint32_t duty_g, uint32_t duty_b);// establece el ciclo de trabajo (duty) para los canales R, G, B del LED RGB

#endif // LED_PWM_H
