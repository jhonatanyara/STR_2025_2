#ifndef KEYPAD_H
#define KEYPAD_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Configuración de Pines (Usando el esquema seguro GPIO13-32) ---
// Cambia estos números si tu cableado es distinto, pero mantén el orden:
// Fila 1, Fila 2, Fila 3, Fila 4.
#define R1_PIN GPIO_NUM_13
#define R2_PIN GPIO_NUM_12
#define R3_PIN GPIO_NUM_14
#define R4_PIN GPIO_NUM_27

// Columna 1, Columna 2, Columna 3, Columna 4
#define C1_PIN GPIO_NUM_26
#define C2_PIN GPIO_NUM_25
#define C3_PIN GPIO_NUM_33
#define C4_PIN GPIO_NUM_32

// Declaración de funciones
void keypad_init(void);
char keypad_get_key(void);

#endif // KEYPAD_H