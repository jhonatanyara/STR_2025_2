#include "button_control.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUTTON_CTRL";

// definimos el boton que vamos a usar como el BOOT de la placa
#define BUTTON_GPIO_PIN GPIO_NUM_0// Pin del botón (BOOT en muchas placas ESP32)
#define DEBOUNCE_TIME_MS 50 // Tiempo para antirrebote

//  VARIABLES ESTATICAS CLAVE 
static bool button_toggle_flag = false; // Bandera para notificar al main
static int last_button_level = 1;      // Estado anterior (1=Liberado, 0=Presionado)

/**
 * @brief Tarea que monitorea el estado del botón.
 * @param arg No utilizado.
 */
void button_monitor_task(void *arg)
{
    // Configure el pin como entrada y habilite la resistencia pull-up
    gpio_set_direction(BUTTON_GPIO_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO_PIN, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "Tarea de monitoreo de botón iniciada en GPIO %d", BUTTON_GPIO_PIN);

    while (1) {
        // Lee el estado del botón (0 es presionado si usa pull-up)
        int current_level = gpio_get_level(BUTTON_GPIO_PIN);

        // Si el estado actual es DIFERENTE al estado anterior
        if (current_level != last_button_level) {
            // Se detectó un cambio (potencialmente rebote, esperamos un poco)
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            current_level = gpio_get_level(BUTTON_GPIO_PIN); // Releer después del debounce

            // Si el estado SIGUE siendo diferente (pulsación real)
            if (current_level != last_button_level) {
                // Se detecta el flanco de LIBERACIÓN (de 0 a 1), que marca el final de la pulsación
                if (current_level == 1 && last_button_level == 0) {
                    button_toggle_flag = true; // Se detectó un TOGGLE
                    ESP_LOGI(TAG, "¡Botón pulsado y LIBERADO! Monitoreo cambiará.");
                }

                last_button_level = current_level; // Actualizar el estado anterior
            }
        }
        
        // Espera un corto periodo para liberar el CPU y continuar el monitoreo
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}


// Función de inicialización declarada en button_control.h
void button_init(void)
{
    // Crea la tarea para el monitoreo del botón
    // Asegúrese de que el tamaño de pila sea adecuado (2048 está bien para esta tarea simple)
    xTaskCreate(button_monitor_task, "button_mon", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Sistema de control de botones inicializado.");
}


// IMPLEMENTACION DE LA NUEVA FUNCION
bool button_was_toggled(void)
{
    if (button_toggle_flag) {
        button_toggle_flag = false; // Limpiar la bandera para esperar la siguiente pulsación
        return true;
    }
    return false;
}