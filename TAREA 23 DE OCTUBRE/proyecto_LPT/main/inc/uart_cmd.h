#ifndef UART_CMD_H
#define UART_CMD_H

#include "esp_err.h"

/**
 * @brief Initializes the UART command interface.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t uart_cmd_init(void);

#endif // UART_CMD_H