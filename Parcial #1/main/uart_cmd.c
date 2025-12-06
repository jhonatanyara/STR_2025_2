#include "uart_cmd.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "potenciometro.h"

static const char *TAG = "UART_CMD";

#define UART_PORT_NUM      UART_NUM_0 // Puerto UART por defecto (terminal serial)
#define BUF_SIZE           1024 // Tamaño del buffer de recepción
#define MIN_DELAY_MS       100 // Mínimo delay permitido
#define MAX_DELAY_MS       5000 // Máximo delay permitido
#define DEFAULT_DELAY_MS   500 // Valor por defecto del delay

// Variable estática para el delay (encapsulada en este módulo)
static uint32_t s_update_delay_ms = DEFAULT_DELAY_MS;
// Flag para reporte periódico del voltaje del potenciómetro (encapsulado en este módulo)
static bool s_pot_reporting_enabled = false;

/* Thresholds por defecto para los LEDs (en °C). Estos se pueden actualizar por UART.
 * Valores iniciales: R 0-15, G 10-30, B 40-50
 */
static float s_r_min = 0.0f;
static float s_r_max = 15.0f;
static float s_g_min = 10.0f;
static float s_g_max = 30.0f;
static float s_b_min = 40.0f;
static float s_b_max = 50.0f;

/* Getters */
float uart_cmd_get_r_min(void) { return s_r_min; }
float uart_cmd_get_r_max(void) { return s_r_max; }
float uart_cmd_get_g_min(void) { return s_g_min; }
float uart_cmd_get_g_max(void) { return s_g_max; }
float uart_cmd_get_b_min(void) { return s_b_min; }
float uart_cmd_get_b_max(void) { return s_b_max; }

/* Setters con comprobación básica: min < max */
esp_err_t uart_cmd_set_r_min(float v) { if (!(v < s_r_max)) return ESP_ERR_INVALID_ARG; s_r_min = v; return ESP_OK; }
esp_err_t uart_cmd_set_r_max(float v) { if (!(v > s_r_min)) return ESP_ERR_INVALID_ARG; s_r_max = v; return ESP_OK; }
esp_err_t uart_cmd_set_g_min(float v) { if (!(v < s_g_max)) return ESP_ERR_INVALID_ARG; s_g_min = v; return ESP_OK; }
esp_err_t uart_cmd_set_g_max(float v) { if (!(v > s_g_min)) return ESP_ERR_INVALID_ARG; s_g_max = v; return ESP_OK; }
esp_err_t uart_cmd_set_b_min(float v) { if (!(v < s_b_max)) return ESP_ERR_INVALID_ARG; s_b_min = v; return ESP_OK; }
esp_err_t uart_cmd_set_b_max(float v) { if (!(v > s_b_min)) return ESP_ERR_INVALID_ARG; s_b_max = v; return ESP_OK; }

// Tarea que envía periódicamente por UART el voltaje del potenciómetro cuando está habilitado
static void pot_report_task(void *arg) {
    (void)arg;
    char response[64];
    while (1) {
        if (s_pot_reporting_enabled) {
            float v = potenciometro_get_voltage();
            // Formatear y enviar el voltaje
            int n = snprintf(response, sizeof(response), "POT_VOLTAGE: %.3f V\n", v);
            if (n > 0) {
                uart_write_bytes(UART_PORT_NUM, response, n);
            }
            // Esperar el intervalo configurado
            vTaskDelay(pdMS_TO_TICKS(uart_cmd_get_update_delay()));
        } else {
            // Pequeña espera para no consumir CPU
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/**
 * @brief Tarea para leer comandos de la UART (terminal serial).
 * @param arg No utilizado.
 */
esp_err_t uart_cmd_set_update_delay(uint32_t delay_ms) { // Establece el tiempo de actualización (el comando es SET_DELAY <ms>)
    if (delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS) {
        return ESP_ERR_INVALID_ARG;
    }
    s_update_delay_ms = delay_ms;
    return ESP_OK;
}

uint32_t uart_cmd_get_update_delay(void) {
    return s_update_delay_ms;
}

static void process_command(const char* cmd) {
    // Remover posibles caracteres de nueva línea
    char clean_cmd[BUF_SIZE];
    strncpy(clean_cmd, cmd, BUF_SIZE - 1);
    clean_cmd[BUF_SIZE - 1] = '\0';
    
    // Remover \r\n o \n del final
    char* p = strchr(clean_cmd, '\r');
    if (p) *p = '\0';
    p = strchr(clean_cmd, '\n');
    if (p) *p = '\0';

    // Procesar comando SET_DELAY
    if (strncmp(clean_cmd, "SET_DELAY ", 10) == 0) {
        char* value_str = clean_cmd + 10;
        uint32_t new_delay = atoi(value_str);
        
        if (uart_cmd_set_update_delay(new_delay) == ESP_OK) {
            char response[64];
            snprintf(response, sizeof(response), "Update delay set to: %lu ms\n", new_delay);
            uart_write_bytes(UART_PORT_NUM, response, strlen(response));
        } else {
            char response[64];
            snprintf(response, sizeof(response), "Error: Delay must be between %d and %d ms\n", 
                    MIN_DELAY_MS, MAX_DELAY_MS);
            uart_write_bytes(UART_PORT_NUM, response, strlen(response));
        }
    }
    // Mantener el comando status existente
    else if (strcmp(clean_cmd, "status") == 0) {
        char response[64];
        snprintf(response, sizeof(response), "STATUS: OK (Update delay: %lu ms)\n", s_update_delay_ms);
        uart_write_bytes(UART_PORT_NUM, response, strlen(response));
    }
    // Habilitar reporte periódico del voltaje del potenciómetro
    else if (strcmp(clean_cmd, "POT_ON") == 0) {
        s_pot_reporting_enabled = true;
        uart_write_bytes(UART_PORT_NUM, "Potenciómetro: reporte periódico ACTIVADO\n", 40);
    }
    // Deshabilitar reporte periódico
    else if (strcmp(clean_cmd, "POT_OFF") == 0) {
        s_pot_reporting_enabled = false;
        uart_write_bytes(UART_PORT_NUM, "Potenciómetro: reporte periódico DESACTIVADO\n", 42);
    }
    // Lectura única del voltaje del potenciómetro
    else if (strcmp(clean_cmd, "POT_READ") == 0) {
        float v = potenciometro_get_voltage();
        char response2[64];
        int n = snprintf(response2, sizeof(response2), "POT_VOLTAGE: %.3f V\n", v);
        if (n > 0) uart_write_bytes(UART_PORT_NUM, response2, n);
    }
    // Comandos para setear thresholds: R_MIN <val>, R_MAX <val>, G_MIN, G_MAX, B_MIN, B_MAX
    else if (strncmp(clean_cmd, "R_MIN ", 6) == 0) {
        float val = strtof(clean_cmd + 6, NULL);
        if (uart_cmd_set_r_min(val) == ESP_OK) {
            char resp[64]; int n = snprintf(resp, sizeof(resp), "R_MIN set to %.2f C\n", val); if (n>0) uart_write_bytes(UART_PORT_NUM, resp, n);
        } else uart_write_bytes(UART_PORT_NUM, "Error: R_MIN must be < R_MAX\n", 27);
    }
    else if (strncmp(clean_cmd, "R_MAX ", 6) == 0) {
        float val = strtof(clean_cmd + 6, NULL);
        if (uart_cmd_set_r_max(val) == ESP_OK) {
            char resp[64]; int n = snprintf(resp, sizeof(resp), "R_MAX set to %.2f C\n", val); if (n>0) uart_write_bytes(UART_PORT_NUM, resp, n);
        } else uart_write_bytes(UART_PORT_NUM, "Error: R_MAX must be > R_MIN\n", 27);
    }
    else if (strncmp(clean_cmd, "G_MIN ", 6) == 0) {
        float val = strtof(clean_cmd + 6, NULL);
        if (uart_cmd_set_g_min(val) == ESP_OK) {
            char resp[64]; int n = snprintf(resp, sizeof(resp), "G_MIN set to %.2f C\n", val); if (n>0) uart_write_bytes(UART_PORT_NUM, resp, n);
        } else uart_write_bytes(UART_PORT_NUM, "Error: G_MIN must be < G_MAX\n", 27);
    }
    else if (strncmp(clean_cmd, "G_MAX ", 6) == 0) {
        float val = strtof(clean_cmd + 6, NULL);
        if (uart_cmd_set_g_max(val) == ESP_OK) {
            char resp[64]; int n = snprintf(resp, sizeof(resp), "G_MAX set to %.2f C\n", val); if (n>0) uart_write_bytes(UART_PORT_NUM, resp, n);
        } else uart_write_bytes(UART_PORT_NUM, "Error: G_MAX must be > G_MIN\n", 27);
    }
    else if (strncmp(clean_cmd, "B_MIN ", 6) == 0) {
        float val = strtof(clean_cmd + 6, NULL);
        if (uart_cmd_set_b_min(val) == ESP_OK) {
            char resp[64]; int n = snprintf(resp, sizeof(resp), "B_MIN set to %.2f C\n", val); if (n>0) uart_write_bytes(UART_PORT_NUM, resp, n);
        } else uart_write_bytes(UART_PORT_NUM, "Error: B_MIN must be < B_MAX\n", 27);
    }
    else if (strncmp(clean_cmd, "B_MAX ", 6) == 0) {
        float val = strtof(clean_cmd + 6, NULL);
        if (uart_cmd_set_b_max(val) == ESP_OK) {
            char resp[64]; int n = snprintf(resp, sizeof(resp), "B_MAX set to %.2f C\n", val); if (n>0) uart_write_bytes(UART_PORT_NUM, resp, n);
        } else uart_write_bytes(UART_PORT_NUM, "Error: B_MAX must be > B_MIN\n", 27);
    }
}

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
            process_command((char *)data);
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
    // Crea la tarea para el reporte del potenciómetro
    xTaskCreate(pot_report_task, "pot_report", 2048, NULL, 5, NULL);//el comando es POT_REPORT
    
    ESP_LOGI(TAG, "Sistema de comandos UART inicializado a 115200 baudios.");
    
    return ESP_OK;
}