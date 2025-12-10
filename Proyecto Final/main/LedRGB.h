#ifndef LEDRGB_H
#define LEDRGB_H

#include <stdbool.h>

// Inicializa los pines del LED
void led_rgb_init(void);

// Cambia el color según el estado de bloqueo
// is_locked = true  -> ROJO
// is_locked = false -> VERDE
void led_rgb_update(bool is_locked);

#endif