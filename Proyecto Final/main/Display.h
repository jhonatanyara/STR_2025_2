#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "driver/gpio.h"

// Configuración de Pines I2C (OLED)
#define I2C_MASTER_SCL_IO    GPIO_NUM_22
#define I2C_MASTER_SDA_IO    GPIO_NUM_21
#define OLED_I2C_ADDRESS     0x3C  // Dirección estándar

// Funciones públicas
void display_init(void);
// Agregamos el parámetro 'float temp' al final
void display_update_ui(const char *status, const char *password, int motor_percent, float temp);

#endif // DISPLAY_H