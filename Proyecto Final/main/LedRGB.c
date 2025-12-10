#include "LedRGB.h"
#include "driver/gpio.h"

// --- PINES ELEGIDOS DE TU LISTA ---
#define LED_PIN_RED   GPIO_NUM_4   
#define LED_PIN_GREEN GPIO_NUM_18 

void led_rgb_init(void) {
    // Configurar ROJO
    gpio_reset_pin(LED_PIN_RED);
    gpio_set_direction(LED_PIN_RED, GPIO_MODE_OUTPUT);

    // Configurar VERDE
    gpio_reset_pin(LED_PIN_GREEN);
    gpio_set_direction(LED_PIN_GREEN, GPIO_MODE_OUTPUT);

    // Iniciar apagados (Para LED Cátodo Común)
    gpio_set_level(LED_PIN_RED, 0);
    gpio_set_level(LED_PIN_GREEN, 0);
}

void led_rgb_update(bool is_locked) {
    if (is_locked) {
        // MODO BLOQUEADO: Rojo ON, Verde OFF
        gpio_set_level(LED_PIN_RED, 1);
        gpio_set_level(LED_PIN_GREEN, 0);
    } else {
        // MODO DESBLOQUEADO: Rojo OFF, Verde ON
        gpio_set_level(LED_PIN_RED, 0);
        gpio_set_level(LED_PIN_GREEN, 1);
    }
}