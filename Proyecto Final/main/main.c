#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <time.h>
#include <sys/time.h>
#include "LedRGB.h"


// --- TUS LIBRERÍAS DE HARDWARE ---
#include "Motor.h"
#include "Sensor.h"
#include "Temp_LM35.h"
#include "Display.h"
#include "Keypad.h"

// --- TUS LIBRERÍAS DE INTERNET ---
#include "wifi_app.h"
#include "http_server.h"

static const char *TAG = "MAIN_APP";

// ==========================================================
// 1. VARIABLES GLOBALES (Compartidas con Web y Hardware)
// ==========================================================
// Variables de Lógica de Control
int system_mode = 0;        // 0:Manual, 1:Auto, 2:Prog
int manual_pwm_val = 0;     // Valor seteado desde la Web
float current_temp = 0.0;
bool pir_state = false;
int current_pwm_output = 0; // Lo que realmente va al motor
float auto_tmin = 20.0;
float auto_tmax = 30.0;

// Estructura para Horarios (Requerida por http_server.c)
typedef struct {
    bool active;
    int start_hour;
    int end_hour;
    float t_zero;
    float t_hundred;
} schedule_t;

schedule_t schedules[3] = {
    {false, 8, 12, 20.0, 30.0},
    {false, 14, 18, 22.0, 32.0},
    {false, 20, 23, 18.0, 25.0}
};

// Variables de SEGURIDAD (Keypad)
bool is_locked = true;       // El sistema inicia bloqueado
char input_buffer[10] = "";  // Buffer para guardar la clave tecleada
const char MASTER_PASS[] = "1234"; // CLAVE MAESTRA

// ==========================================================
// 2. FUNCIÓN DE CARGA DE DATOS (NVS)
// ==========================================================
void load_settings_from_nvs() {
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        int32_t val = 0;
        if(nvs_get_i32(my_handle, "sys_mode", &val) == ESP_OK) system_mode = val;
        if(nvs_get_i32(my_handle, "man_pwm", &val) == ESP_OK) manual_pwm_val = val;
        if(nvs_get_i32(my_handle, "auto_tmin", &val) == ESP_OK) auto_tmin = val / 100.0;
        if(nvs_get_i32(my_handle, "auto_tmax", &val) == ESP_OK) auto_tmax = val / 100.0;
        
        // Cargar horarios (simplificado para brevedad)
        char key[16];
        for(int i=0; i<3; i++) {
             if(nvs_get_i32(my_handle, (sprintf(key,"sch%d_act",i),key), &val) == ESP_OK) schedules[i].active = val;
             if(nvs_get_i32(my_handle, (sprintf(key,"sch%d_sh",i),key), &val) == ESP_OK) schedules[i].start_hour = val;
             if(nvs_get_i32(my_handle, (sprintf(key,"sch%d_eh",i),key), &val) == ESP_OK) schedules[i].end_hour = val;
             if(nvs_get_i32(my_handle, (sprintf(key,"sch%d_t0",i),key), &val) == ESP_OK) schedules[i].t_zero = val / 100.0;
             if(nvs_get_i32(my_handle, (sprintf(key,"sch%d_t1",i),key), &val) == ESP_OK) schedules[i].t_hundred = val / 100.0;
        }
        nvs_close(my_handle);
    }
}

// ==========================================================
// 3. TAREA PRINCIPAL (Hardware + Lógica + Display)
// ==========================================================
void system_control_task(void *pvParameters)
{
    char key;
    while (1) {
        // --- A. LEER ENTRADAS ---
        pir_state = sensors_get_pir_state(); // Usamos tu librería Sensor.h
        current_temp = temp_sensor_read_celsius(); // Usamos tu librería Temp_LM35.h
        key = keypad_get_key(); // Usamos tu librería Keypad.h
        

        // --- B. LÓGICA DE TECLADO (SEGURIDAD) ---
        if (key != '\0') {
            ESP_LOGI(TAG, "Tecla: %c", key);
            
            if (key == '*') { 
                // Asterisco: Borrar / Bloquear
                memset(input_buffer, 0, sizeof(input_buffer));
                is_locked = true; 
            } 
            else if (key == '#') {
                // Numeral: Confirmar contraseña
                if (strcmp(input_buffer, MASTER_PASS) == 0) {
                    is_locked = false; // ¡DESBLOQUEADO!
                    memset(input_buffer, 0, sizeof(input_buffer));
                } else {
                    // Clave incorrecta
                    memset(input_buffer, 0, sizeof(input_buffer));
                    // Aquí podrías poner un mensaje de error temporal en el OLED si quisieras
                }
            } 
            else {
                // Números: Agregar al buffer
                int len = strlen(input_buffer);
                if (len < 8) {
                    input_buffer[len] = key;
                    input_buffer[len+1] = '\0';
                }
            }
        }

        // --- C. LÓGICA DE CONTROL (VENTILADOR) ---
        int target_pwm = 0;

        if (is_locked) {
            // SI ESTÁ BLOQUEADO: Motor apagado siempre
            target_pwm = 0;
        } 
        else {
            // SI ESTÁ DESBLOQUEADO: Usar lógica normal
            
            // 1. MANUAL
            if (system_mode == 0) target_pwm = manual_pwm_val;
            
            // 2. AUTO
            else if (system_mode == 1) {
                if (!pir_state) target_pwm = 0;
                else {
                    if (current_temp <= auto_tmin) target_pwm = 0;
                    else if (current_temp >= auto_tmax) target_pwm = 100;
                    else target_pwm = (int)((current_temp - auto_tmin) * 100 / (auto_tmax - auto_tmin));
                }
            }
            
            // 3. PROGRAMADO
            else if (system_mode == 2) {
                time_t now; struct tm timeinfo; time(&now); localtime_r(&now, &timeinfo);
                int h = timeinfo.tm_hour;
                target_pwm = 0;
                if (pir_state) {
                    for(int i=0; i<3; i++) {
                        if(schedules[i].active && h >= schedules[i].start_hour && h < schedules[i].end_hour) {
                            float t0 = schedules[i].t_zero; float t100 = schedules[i].t_hundred;
                            if (current_temp <= t0) target_pwm = 0;
                            else if (current_temp >= t100) target_pwm = 100;
                            else target_pwm = (int)((current_temp - t0) * 100 / (t100 - t0));
                            break;
                        }
                    }
                }
            }
        }

        // Aplicar al Motor
        motor_set_speed_percent(target_pwm); // Usamos tu librería Motor.h
        current_pwm_output = target_pwm;

        led_rgb_update(is_locked);

        // --- D. ACTUALIZAR PANTALLA OLED ---
        // Usamos tu librería Display.h
        if (is_locked) {
            display_update_ui("BLOQUEADO", input_buffer, 0, current_temp);
        } else {
            // Mostrar modo en pantalla
            char mode_str[10];
            if(system_mode==0) strcpy(mode_str, "MANUAL");
            else if(system_mode==1) strcpy(mode_str, "AUTO");
            else strcpy(mode_str, "PROG");
            
            display_update_ui(mode_str, "OK", target_pwm, current_temp);
        }

        // Pequeño delay para no saturar la CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==========================================================
// 4. APP MAIN
// ==========================================================
void app_main(void)
{
    // 1. NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }
    load_settings_from_nvs();

    // 2. INICIALIZAR HARDWARE (Tus librerías)
    motor_init();
    sensors_init();   // PIR
    temp_sensor_init(); // LM35
    display_init();     // OLED
    keypad_init();      // Teclado
    led_rgb_init();

    // 3. INICIALIZAR INTERNET
    wifi_app_start(); // Esto arranca el WiFi y luego el WebServer automáticamente

    // 4. INICIALIZAR TAREA DE CONTROL
    // La fijamos al Core 1 para dejar el Core 0 al WiFi
    xTaskCreatePinnedToCore(system_control_task, "SystemCtrl", 4096, NULL, 5, NULL, 1);
    
    ESP_LOGI(TAG, "SISTEMA INICIADO COMPLETO");
}