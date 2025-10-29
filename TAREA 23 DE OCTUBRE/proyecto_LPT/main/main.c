#include <stdio.h>
#include <string.h> // Necesario para la manipulación de strings si decides pasar comandos directos
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

// Incluye los archivos de cabecera de los sensores
#include "termistor.h"
#include "potenciometro.h"

// Incluye el controlador LED PWM
#include "led_pwm.h"
// Incluye el controlador de comandos UART (para thresholds y comandos)
#include "uart_cmd.h"
// Incluye el controlador de botones
#include "button_control.h" // Se mantiene para compatibilidad con el entorno de VS Code

// Etiqueta para el logging
static const char *TAG = "MAIN_APP";

// Handle global para la unidad ADC
static adc_oneshot_unit_handle_t adc1_unit_handle;

// Variables globales para los límites de temperatura (Thresholds)
// NOTA: Estos valores deben ser actualizados por la tarea UART
// Requisitos: Rojo (0-15), Verde (10-30), Azul (40-50)
float R_MIN_THRESHOLD = 0.0f;
float R_MAX_THRESHOLD = 15.0f;
float G_MIN_THRESHOLD = 10.0f;
float G_MAX_THRESHOLD = 30.0f;
float B_MIN_THRESHOLD = 40.0f;
float B_MAX_THRESHOLD = 50.0f;

// 🌟 NUEVA VARIABLE DE ESTADO PARA EL MONITOREO POR UART 🌟
// Esta variable controla si la impresión de datos por UART está activa.
static bool is_monitoring_enabled = true;


/**
 * @brief Función auxiliar para mapear la temperatura a un canal de color (R, G, o B).
 * * Basado en los requisitos: El color depende de la temperatura.
 * * @param temp_celsius Temperatura actual medida.
 * @param min_temp Límite inferior para que el color esté activo (0% intensidad).
 * @param max_temp Límite superior para que el color esté al 100% de intensidad.
 * @return float Valor de intensidad normalizado (0.0 a 1.0) para el canal de color.
 */
float map_temp_to_color_intensity(float temp_celsius, float min_temp, float max_temp) {
    if (temp_celsius <= min_temp) {
        return 0.0f;
    }
    if (temp_celsius >= max_temp) {
        return 1.0f;
    }
    // Mapeo lineal entre min_temp y max_temp
    return (temp_celsius - min_temp) / (max_temp - min_temp);
}

// 🌟 NUEVA FUNCIÓN PARA MANEJAR LOS COMANDOS DE ACTIVACIÓN/DESACTIVACIÓN 🌟
void handle_monitor_command(char *command) {
    if (strcmp(command, "ENABLE_MONITOR") == 0) {
        is_monitoring_enabled = true;
        ESP_LOGI(TAG, "Monitoreo por UART: ACTIVADO");
    } else if (strcmp(command, "DISABLE_MONITOR") == 0) {
        is_monitoring_enabled = false;
        ESP_LOGI(TAG, "Monitoreo por UART: DESACTIVADO");
    } 
    // NOTA: Aquí puedes agregar llamadas a la función de tu librería uart_cmd
    // para manejar los thresholds, si aplica.
}


void app_sensors_task(void *pvParameters) {
    // ----------------------------------------------------------------------
    // 1. Inicialización de la Unidad ADC
    // ----------------------------------------------------------------------
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1, // Usaremos la unidad 1
    };
    
    // Inicializar la unidad y obtener el handle.
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_unit_handle));
    ESP_LOGI(TAG, "Unidad ADC inicializada.");
    
    // ----------------------------------------------------------------------
    // 2. Inicialización de los Canales y Periféricos
    // ----------------------------------------------------------------------
    termistor_init(adc1_unit_handle);
    potenciometro_init(adc1_unit_handle);
    
    // Inicialización de LED PWM
    led_pwm_init();
    ESP_LOGI(TAG, "Controlador LED PWM inicializado.");

    // NOTA: Se ha eliminado la inicialización del botón.


    float current_temperature = 0;
    float normalized_pot_value = 0; // Intensidad de 0.0 a 1.0


    while (1) {
        // ----------------------------------------------------------------------
        // 3. Lectura de Sensores
        // ----------------------------------------------------------------------
        current_temperature = termistor_get_temperature_celsius();
        // Obtener el valor de intensidad del potenciómetro
        normalized_pot_value = potenciometro_get_normalized_value(); 

        // ----------------------------------------------------------------------
        // 4. Lógica de Control del LED RGB (SIEMPRE ACTIVA)
        // ----------------------------------------------------------------------
        
        // La funcionalidad de control del LED NO SE ELIMINA, se ejecuta siempre.
        float r_color_intensity = map_temp_to_color_intensity(current_temperature, R_MIN_THRESHOLD, R_MAX_THRESHOLD);
        float g_color_intensity = map_temp_to_color_intensity(current_temperature, G_MIN_THRESHOLD, G_MAX_THRESHOLD);
        float b_color_intensity = map_temp_to_color_intensity(current_temperature, B_MIN_THRESHOLD, B_MAX_THRESHOLD);

        uint32_t final_r_duty = (uint32_t)(r_color_intensity * normalized_pot_value * 100.0f);
        uint32_t final_g_duty = (uint32_t)(g_color_intensity * normalized_pot_value * 100.0f);
        uint32_t final_b_duty = (uint32_t)(b_color_intensity * normalized_pot_value * 100.0f);

        if (final_r_duty > 100) final_r_duty = 100;
        if (final_g_duty > 100) final_g_duty = 100;
        if (final_b_duty > 100) final_b_duty = 100;
        
        led_pwm_set_rgb(final_r_duty, final_g_duty, final_b_duty);


        // ----------------------------------------------------------------------
        // 5. Lógica de Impresión (CONTROLADA POR 'is_monitoring_enabled')
        // ----------------------------------------------------------------------
        
        if (is_monitoring_enabled) { //  Condición para activar/desactivar
            ESP_LOGI(TAG, "--- Lectura ---");
            ESP_LOGI(TAG, "Temperatura del Termistor: %.2f C", current_temperature);
            ESP_LOGI(TAG, "Valor del Potenciómetro (Brillo): %.2f (0.0 a 1.0)", normalized_pot_value);
            ESP_LOGI(TAG, "LED RGB (R, G, B): %" PRIu32 "%%, %" PRIu32 "%%, %" PRIu32 "%%", 
                        final_r_duty, final_g_duty, final_b_duty);
        } else {
             // Opcional: Solo imprimir el estado una vez mientras está desactivado
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // Espera 500 milisegundos
        
    }
}

void app_main(void) {
    // ----------------------------------------------------------------------
    // 1. Inicialización de UART CMD (para cambiar los thresholds y recibir comandos)
    // ----------------------------------------------------------------------
    // Al iniciar, la tarea UART de tu librería debe empezar a escuchar.
    uart_cmd_init();
    
    // ----------------------------------------------------------------------
    // 2. Tarea de gestión de comandos UART
    // ----------------------------------------------------------------------
    // Aquí se asume que la librería 'uart_cmd' recibe comandos y los pone
    // en una cola (queue) o que podemos verificar si hay un comando disponible.
    // **Si tu uart_cmd maneja una cola de comandos, agrega aquí una tarea para procesarla.**
    
    // EJEMPLO de TAREA de procesamiento de comandos (si no está ya en uart_cmd.c):
    /*
    xTaskCreate(uart_command_processor_task, "uart_proc", 4096, NULL, 5, NULL);
    */

    // ----------------------------------------------------------------------
    // 3. Crear la tarea de lectura y control de sensores
    // ----------------------------------------------------------------------
    xTaskCreate(app_sensors_task, "sensors_task", 4096, NULL, 5, NULL);
}


