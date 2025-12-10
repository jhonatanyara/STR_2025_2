#include "Keypad.h"
#include "esp_log.h"

static const char *TAG = "KEYPAD";

// 1. La Matriz (Idéntica a tu código Arduino)
const char keys[4][4] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

// 2. Arreglos de Pines (Mapeo directo como en Arduino)
// El índice 0 de rowPins corresponde a la Fila 0 de la matriz 'keys'
const gpio_num_t rowPins[4] = { R1_PIN, R2_PIN, R3_PIN, R4_PIN };
const gpio_num_t colPins[4] = { C1_PIN, C2_PIN, C3_PIN, C4_PIN };

void keypad_init(void)
{
    // Configurar FILAS como SALIDAS (OUTPUT)
    // Inicialmente las ponemos en ALTO (HIGH) para que no activen nada
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(rowPins[i]);
        gpio_set_direction(rowPins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(rowPins[i], 1); 
    }

    // Configurar COLUMNAS como ENTRADAS con PULL-UP (INPUT_PULLUP)
    // Esto es clave: normalmente leerán 1. Si presionas tecla, leerán 0.
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(colPins[i]);
        gpio_set_direction(colPins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(colPins[i], GPIO_PULLUP_ONLY);
    }
    
    ESP_LOGI(TAG, "Keypad inicializado al estilo Arduino.");
}

char keypad_get_key(void)
{
    char key = '\0'; // Carácter nulo por defecto

    // Algoritmo de Escaneo (Idéntico a la librería Keypad.h de Arduino)
    for (int r = 0; r < 4; r++) {
        // 1. Activar Fila actual (Ponerla en LOW)
        gpio_set_level(rowPins[r], 0);

        // 2. Revisar todas las Columnas
        for (int c = 0; c < 4; c++) {
            // Si la columna lee 0 (LOW), significa que el circuito se cerró
            if (gpio_get_level(colPins[c]) == 0) {
                
                // Pequeño delay para evitar rebotes (Debounce)
                vTaskDelay(pdMS_TO_TICKS(20)); 

                // Confirmamos si sigue presionada
                if (gpio_get_level(colPins[c]) == 0) {
                    key = keys[r][c]; // Obtenemos el carácter de la matriz

                    // Esperamos a que el usuario suelte la tecla (para no repetir)
                    while (gpio_get_level(colPins[c]) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    
                    // Restauramos la fila a HIGH antes de salir
                    gpio_set_level(rowPins[r], 1);
                    return key; 
                }
            }
        }

        // 3. Desactivar Fila actual (Volver a ponerla en HIGH)
        gpio_set_level(rowPins[r], 1);
    }

    return '\0'; // Ninguna tecla presionada
}