#include "Sensor.h" // <-- Incluye el header en plural
#include "esp_log.h"
#include "driver/gpio.h" 

static const char *TAG = "SENSOR"; // Etiqueta del Log en singular

void sensors_init(void) {
    gpio_reset_pin(PIR_PIN);
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_PIN, GPIO_PULLDOWN_ONLY); 
    
    ESP_LOGI(TAG, "Sensor PIR inicializado en GPIO %d", PIR_PIN);
}

bool sensors_get_pir_state(void) {
    return gpio_get_level(PIR_PIN);
}