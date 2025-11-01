#ifndef UART_CMD_H
#define UART_CMD_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Initializes the UART command interface.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t uart_cmd_init(void);// inicializa la interfaz de comandos UART

/**
 * @brief Establece el tiempo de actualización para la lectura de sensores.
 * @param delay_ms Nuevo tiempo de actualización en milisegundos
 * @return ESP_OK si el valor es válido, ESP_ERR_INVALID_ARG si no lo es.
 */
esp_err_t uart_cmd_set_update_delay(uint32_t delay_ms);// establece el tiempo de actualización (el comando es SET_DELAY <ms>)

/**
 * @brief Obtiene el tiempo de actualización actual.
 * @return Tiempo de actualización en milisegundos.
 */
uint32_t uart_cmd_get_update_delay(void);// obtiene el tiempo de actualización actual

/* Setters/Getters para thresholds de colores (valores en °C) */
esp_err_t uart_cmd_set_r_min(float v);
esp_err_t uart_cmd_set_r_max(float v);
esp_err_t uart_cmd_set_g_min(float v);
esp_err_t uart_cmd_set_g_max(float v);
esp_err_t uart_cmd_set_b_min(float v);
esp_err_t uart_cmd_set_b_max(float v);

float uart_cmd_get_r_min(void);
float uart_cmd_get_r_max(void);
float uart_cmd_get_g_min(void);
float uart_cmd_get_g_max(void);
float uart_cmd_get_b_min(void);
float uart_cmd_get_b_max(void);

#endif // UART_CMD_H