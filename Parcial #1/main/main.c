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
// Reemplazamos las variables globales de thresholds por funciones específicas
// que devuelven la intensidad normalizada (0.0 - 1.0) para cada color.
static float get_red_intensity(float temp_celsius)
{
    float min_temp = uart_cmd_get_r_min();
    float max_temp = uart_cmd_get_r_max();
    if (temp_celsius <= min_temp || temp_celsius > max_temp) return 0.0f;
    return (temp_celsius - min_temp) / (max_temp - min_temp);
}

static float get_green_intensity(float temp_celsius)
{
    float min_temp = uart_cmd_get_g_min();
    float max_temp = uart_cmd_get_g_max();
    if (temp_celsius <= min_temp || temp_celsius > max_temp) return 0.0f;
    return (temp_celsius - min_temp) / (max_temp - min_temp);
}

static float get_blue_intensity(float temp_celsius)
{
    float min_temp = uart_cmd_get_b_min();
    float max_temp = uart_cmd_get_b_max();
    if (temp_celsius <= min_temp || temp_celsius > max_temp) return 0.0f;
    return (temp_celsius - min_temp) / (max_temp - min_temp);
}

/**
 * @brief Intensidad para blanco (todas las componentes iguales) entre 50C y 200C
 * Si la temperatura está en el rango (50, 200], devuelve intensidad 0..1.
 * Nota: Asumimos que los valores mayores a 200 se saturan al 100%.
 */
static float get_white_intensity(float temp_celsius)
{
    const float min_temp = 50.0f;
    const float max_temp = 200.0f;
    if (temp_celsius <= min_temp) {
        return 0.0f;
    }
    if (temp_celsius >= max_temp) {
        return 1.0f;
    }
    return (temp_celsius - min_temp) / (max_temp - min_temp);
}

// si la temperatura supera los 50 grados, el LED se enciende blanco al máximo

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
        // Inicializar control de botón (para togglear encendido/apagado del LED)
        button_init();

    // NOTA: Se ha eliminado la inicialización del botón.


    float current_temperature = 0;
    float normalized_pot_value = 0; // Intensidad de 0.0 a 1.0


    // Estado local para saber si el LED está habilitado
    bool led_enabled = true;

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
        float r_color_intensity = get_red_intensity(current_temperature);
        float g_color_intensity = get_green_intensity(current_temperature);
        float b_color_intensity = get_blue_intensity(current_temperature);
        float w_color_intensity = get_white_intensity(current_temperature);

    uint32_t final_r_duty, final_g_duty, final_b_duty;
    if (w_color_intensity > 0.0f) {
        /* Si el blanco está activo, mostramos blanco (todas las componentes iguales)
         * y sobreescribimos las otras componentes. */
        uint32_t white_duty = (uint32_t)(w_color_intensity * normalized_pot_value * 100.0f);
        if (white_duty > 100) white_duty = 100;
        final_r_duty = final_g_duty = final_b_duty = white_duty;
    } else {
        final_r_duty = (uint32_t)(r_color_intensity * normalized_pot_value * 100.0f);
        final_g_duty = (uint32_t)(g_color_intensity * normalized_pot_value * 100.0f);
        final_b_duty = (uint32_t)(b_color_intensity * normalized_pot_value * 100.0f);
    }

        if (final_r_duty > 100) final_r_duty = 100;
        if (final_g_duty > 100) final_g_duty = 100;
        if (final_b_duty > 100) final_b_duty = 100;
        
        // Comprobar si el botón fue pulsado (toggle)
        if (button_was_toggled()) {
            led_enabled = !led_enabled;
            if (!led_enabled) {
                // Apagar LED inmediatamente
                led_pwm_set_rgb(0, 0, 0);
                ESP_LOGI(TAG, "LED APAGADO por pulsación de botón");
            } else {
                // Encender con el último duty calculado
                led_pwm_set_rgb(final_r_duty, final_g_duty, final_b_duty);
                ESP_LOGI(TAG, "LED ENCENDIDO por pulsación de botón");
            }
        } else {
            // Si está habilitado, actualizar según sensores. Si está deshabilitado, mantener apagado.
            if (led_enabled) {
                led_pwm_set_rgb(final_r_duty, final_g_duty, final_b_duty);
            }
        }
        // 5. Lógica de Impresión (CONTROLADA POR 'is_monitoring_enabled')
        if (is_monitoring_enabled) { //  Condición para activar/desactivar
            ESP_LOGI(TAG, "--- Lectura ---");
            ESP_LOGI(TAG, "Temperatura del Termistor: %.2f C", current_temperature);
            ESP_LOGI(TAG, "Valor del Potenciómetro (Brillo): %.2f (0.0 a 1.0)", normalized_pot_value);
            // Impresión de los niveles de brillo de los LEDs eliminada intencionalmente
        } else {
            // Opcional: Solo imprimir el estado una vez mientras está desactivado
        }

        vTaskDelay(pdMS_TO_TICKS(uart_cmd_get_update_delay())); // Usa el delay configurado por UART
        
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
    // 3. Crear la tarea de lectura y control de sensores
    xTaskCreate(app_sensors_task, "sensors_task", 4096, NULL, 5, NULL);
}

// comandos del serial monitor 
//para variar el tiempo es SET_DELAY <ms>
//para dar el valor del potenciometro es POT_READ
//comandos para variar los thresholds de colores
//rojo R_MIN <valor>, R_MAX <valor>
//verde G_MIN <valor>, G_MAX <valor>
//azul B_MIN <valor>, B_MAX <valor>
//


