#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "Keypad.h"
#include "Motor.h"
#include "Sensor.h" 
#include "driver/gpio.h" // Necesario para usar gpio_set_level

// Pines de los LEDs
#define ACCESS_GRANTED_PIN 19
#define ACCESS_DENIED_PIN 18

static const char *TAG = "MAIN";
const char *CORRECT_PASSWORD = "1234#"; 

// --- VARIABLES GLOBALES COMPARTIDAS ---
// Variable para que la tarea de display sepa qué velocidad mostrar.
volatile int g_motor_speed_percent = 0; 
// Variable de estado del sistema (Fase 1)
bool system_active = false; 

// Variables globales para la lógica de contraseña
char entered_value[6];
uint8_t valIndex = 0;

void status_led_init(void) {
    gpio_reset_pin(ACCESS_GRANTED_PIN);
    gpio_set_direction(ACCESS_GRANTED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ACCESS_DENIED_PIN);
    gpio_set_direction(ACCESS_DENIED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ACCESS_GRANTED_PIN, 0);
    gpio_set_level(ACCESS_DENIED_PIN, 0);
}

void check_password() {
    if (strcmp(entered_value, CORRECT_PASSWORD) == 0) {
        ESP_LOGI(TAG, "ACCESO CONCEDIDO");
        gpio_set_level(ACCESS_GRANTED_PIN, 1);
        gpio_set_level(ACCESS_DENIED_PIN, 0);
        system_active = true; // Activar sistema
    } else {
        ESP_LOGW(TAG, "ACCESO DENEGADO");
        gpio_set_level(ACCESS_GRANTED_PIN, 0);
        gpio_set_level(ACCESS_DENIED_PIN, 1);
        system_active = false; // Mantener sistema bloqueado
    }
    // Reiniciar buffer y índice
    memset(entered_value, 0, sizeof(entered_value));
    valIndex = 0;   
}

/**
 * @brief Tarea encargada de mostrar el porcentaje del motor cada 1 segundo.
 */
void motor_display_task(void *pvParameter) {
    static const char *DISPLAY_TAG = "MOTOR_INFO";
    while (1) {
        // Loguea el valor actual de la velocidad (0-100)
        ESP_LOGI(DISPLAY_TAG, "Velocidad del Ventilador: %d%%", g_motor_speed_percent);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo
    }
}


/**
 * @brief Tarea principal que gestiona la lógica de control (Keypad y PIR).
 */
void access_control_task(void *pvParameter) {
    
    // 1. Inicializar Hardware
    keypad_init();
    status_led_init();
    
    // --- INICIALIZACIÓN FASE 1 ---
    motor_init(); // Inicializar PWM del ventilador
    sensors_init(); // Inicializar PIR
    
    ESP_LOGI(TAG, "Sistema listo. ESTADO: BLOQUEADO. Ingrese clave:");
    memset(entered_value, 0, sizeof(entered_value));

    while (1) {
        // ==========================================
        // 1. LOGICA DEL TECLADO (Siempre funciona)
        // ==========================================
        char key = keypad_get_key();

        if (key != '\0') {
            ESP_LOGI(TAG, "Tecla: %c", key);

            if (key == '#') {
                // Añadimos el terminador # para comparar con "1234#"
                if(valIndex < 5) {
                    entered_value[valIndex++] = key;
                    entered_value[valIndex] = '\0';
                }
                check_password();
            } 
            else if (key == '*') {
                // Tecla '*' para borrar y BLOQUEAR manualmente (opcional)
                if (valIndex > 0) {
                    valIndex--;
                    entered_value[valIndex] = '\0';
                    ESP_LOGI(TAG, "Borrado: %s", entered_value);
                } else {
                    // Si el buffer está vacío y presiono *, bloqueo el sistema manualmente
                    system_active = false;
                    motor_set_speed_percent(0);
                    g_motor_speed_percent = 0; // Actualizamos global
                    ESP_LOGW(TAG, "Sistema BLOQUEADO manualmente");
                }
            } 
            else {
                // Es un número o letra
                if (valIndex < 5) {
                    entered_value[valIndex++] = key;
                    entered_value[valIndex] = '\0';
                    ESP_LOGI(TAG, "Buffer: %s", entered_value);
                } else {
                    ESP_LOGW(TAG, "Buffer lleno");
                }
            }
        }
        
        // ==========================================
        // 2. LOGICA DEL VENTILADOR (Control PIR)
        // ==========================================
        if (system_active) {
            bool movimiento = sensors_get_pir_state();
            
            if (movimiento) {
                // Si hay presencia -> Ventilador al 100%
                motor_set_speed_percent(100); 
                g_motor_speed_percent = 100; // Actualizar global
            } else {
                // Si NO hay presencia -> Ventilador al 0% (Apagado)
                motor_set_speed_percent(0); // <-- CORREGIDO: De 100 a 0 para apagar
                g_motor_speed_percent = 0; // Actualizar global
            }
        } else {
            // El sistema está bloqueado -> Asegurar motor apagado
            motor_set_speed_percent(0);
            g_motor_speed_percent = 0; // Actualizar global
        }
        
        // Delay pequeño para la estabilidad del RTOS
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void app_main(void) {
    // Tarea 1: Control principal (Keypad y Lógica de Detección/Motor)
    xTaskCreate(&access_control_task, "access_control_task", 4096, NULL, 5, NULL);

    // Tarea 2: Display del Porcentaje del Motor (Se ejecuta cada segundo)
    xTaskCreate(&motor_display_task, "motor_display_task", 2048, NULL, 4, NULL);
}
// Explicación del Código:
// El sistema inicia bloqueado, esperando la contraseña "1234#".
// Al ingresar la contraseña correcta, el sistema se activa y el ventilador
// responde al sensor PIR: si detecta movimiento, el ventilador se enciende al 100%,
// si no, se apaga. Si la contraseña es incorrecta, el sistema permanece bloqueado
// los pines del motor son 35 y el otro a tierra, el pwm controla la velocidad del motor.