#ifndef DISPLAY_H
#define DISPLAY_H

#include "driver/gpio.h"

// --- CONFIGURACIÓN DE PINES Y DIRECCIÓN I2C ---
#define I2C_MASTER_SCL_IO    GPIO_NUM_22  // Pin SCL (Clock)
#define I2C_MASTER_SDA_IO    GPIO_NUM_21  // Pin SDA (Data)
#define I2C_MASTER_NUM       I2C_NUM_0    // Puerto I2C 0
#define I2C_MASTER_FREQ_HZ   100000       // Frecuencia I2C
#define LCD_I2C_ADDRESS      0x27         // Dirección I2C (común: 0x27 o 0x3F)

// --- Funciones de Control ---

/**
 * @brief Inicializa el bus I2C y la pantalla LCD 16x2.
 */
void display_init(void);

/**
 * @brief Muestra la contraseña ingresada en la primera línea.
 * Ejemplo: "Clave: ****"
 * @param password Cadena de la contraseña actual.
 */
void display_password(const char *password);

/**
 * @brief Muestra el estado actual del sistema en la segunda línea.
 * Limpia automáticamente los caracteres sobrantes de la línea.
 * @param status_line Cadena con el estado.
 */
void display_status(const char *status_line);

#endif // DISPLAY_H