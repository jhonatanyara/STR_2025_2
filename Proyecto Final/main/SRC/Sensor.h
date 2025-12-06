#ifndef SENSORS_H // <-- Mantiene la 'S' para el header
#define SENSORS_H

#include <stdbool.h>
#include "driver/gpio.h"

// Pin del Sensor PIR (Ajusta según tu conexión)
#define PIR_PIN GPIO_NUM_15 

// Inicializa los sensores
void sensors_init(void);

// Retorna true si detecta movimiento
bool sensors_get_pir_state(void);

#endif // SENSORS_H