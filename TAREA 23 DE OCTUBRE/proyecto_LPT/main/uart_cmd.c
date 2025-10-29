#include "uart_cmd.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "UART_CMD";

#define UART_PORT_NUM      UART_NUM_0
#define BUF_SIZE           1024

/**
 * @brief Tarea para leer comandos de la UART (terminal serial).
 * @param arg No utilizado.
 */
void uart_rx_task(void *arg)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    
    ESP_LOGI(TAG, "Tarea de comandos UART iniciada.");

    while (1) {
        // Lee los datos de la UART
        int len = uart_read_bytes(UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            data[len] = '\0'; // Asegura que la cadena esté terminada en nulo
            ESP_LOGI(TAG, "Comando recibido: %s", (char *) data);

            // Ejemplo simple de procesamiento de comandos
            if (strcmp((char *)data, "status\r\n") == 0 || strcmp((char *)data, "status\n") == 0) {
                uart_write_bytes(UART_PORT_NUM, "STATUS: OK\n", 11);
            }
        }
    }
    free(data);
}

// Función de inicialización declarada en uart_cmd.h
esp_err_t uart_cmd_init(void)
{
    // Configuración básica de UART
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    // Instalar el driver de UART
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    
    // Crea la tarea para el manejo de comandos
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "Sistema de comandos UART inicializado a 115200 baudios.");
    
    return ESP_OK;
}