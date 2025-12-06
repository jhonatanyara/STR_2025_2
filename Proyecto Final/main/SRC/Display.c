#include "display.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h> // Necesario para strlen

static const char *TAG = "LCD_I2C";

// --- CONSTANTES DEL LCD ---
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20
#define LCD_SETDDRAMADDR   0x80

// Banderas para Display Control
#define LCD_DISPLAYON      0x04
#define LCD_CURSOROFF      0x00
#define LCD_BLINKOFF       0x00

// Banderas para Function Set
#define LCD_2LINE          0x08
#define LCD_5x8DOTS        0x00

// Banderas para Backlight (PC8574)
#define LCD_BACKLIGHT      0x08
#define LCD_NOBACKLIGHT    0x00

// --- PINES DEL PCF8574 (Mapeo Típico) ---
#define RS_PIN 0x01 // Bit 0
#define RW_PIN 0x02 // Bit 1
#define EN_PIN 0x04 // Bit 2
#define D4_PIN 0x10 // Bit 4
#define D5_PIN 0x20 // Bit 5
#define D6_PIN 0x40 // Bit 6
#define D7_PIN 0x80 // Bit 7

// Estado actual del backlight
static uint8_t backlight_state = LCD_BACKLIGHT;

/**
 * @brief Envía un byte completo al PCF8574 a través de I2C.
 * Nota: Esta función crea y destruye el handle en cada llamada.
 */
static esp_err_t i2c_write_byte(uint8_t val) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, val | backlight_state, true);
    i2c_master_stop(cmd);
    
    // Timeout de 1000ms
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Envía un nibble (4 bits) más la señal de Enable.
 */
static void send_nibble(uint8_t data_nibble, uint8_t mode) {
    uint8_t data = data_nibble | mode;
    
    // Secuencia Enable: HIGH -> LOW para que el LCD lea el dato
    i2c_write_byte(data | EN_PIN); // Enable HIGH
    ets_delay_us(1);               // Pequeña espera (>450ns)
    i2c_write_byte(data & ~EN_PIN); // Enable LOW
    ets_delay_us(50);              // Espera para que el LCD procese
}

/**
 * @brief Divide un byte en dos nibbles y los envía (Modo 4 bits).
 */
static void send_byte(uint8_t value, uint8_t mode) {
    uint8_t high_nibble = value & 0xF0;
    uint8_t low_nibble = (value << 4) & 0xF0;

    send_nibble(high_nibble, mode);
    send_nibble(low_nibble, mode);
}

/**
 * @brief Envía un comando al LCD (RS = 0).
 */
static void lcd_command(uint8_t command) {
    send_byte(command, 0); 
}

/**
 * @brief Envía un dato (carácter) al LCD (RS = 1).
 */
static void lcd_write_char(uint8_t data) {
    send_byte(data, RS_PIN); 
}

/**
 * @brief Inicializa el hardware I2C en el ESP32.
 */
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// --- FUNCIONES PÚBLICAS ---

void display_init(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C Bus inicializado en SDA: %d, SCL: %d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // Secuencia de inicialización especial para modo 4-bits
    vTaskDelay(pdMS_TO_TICKS(50)); // Esperar >40ms después de VCC

    // 1. Reset / Try to set 8-bit mode (3 veces)
    // Nota: enviamos solo el nibble superior manualmente porque aún no estamos en modo 4 bits seguro
    send_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5)); // >4.1ms
    
    send_nibble(0x30, 0);
    ets_delay_us(150); // >100us
    
    send_nibble(0x30, 0);
    
    // 2. Set to 4-bit mode
    send_nibble(0x20, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // A partir de aquí ya podemos usar lcd_command (que envía 2 nibbles)
    
    // 3. Configuración: 2 líneas, 5x8 puntos
    lcd_command(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS);

    // 4. Display Control: Encender display, apagar cursor
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);

    // 5. Clear Display
    lcd_command(LCD_CLEARDISPLAY);
    vTaskDelay(pdMS_TO_TICKS(2)); // Clear display toma más tiempo (>1.52ms)

    // 6. Entry Mode: Increment, No Shift
    lcd_command(LCD_ENTRYMODESET | 0x02);

    ESP_LOGI(TAG, "LCD 16x2 inicializado");
}

static void lcd_print(const char *str) {
    while (*str) {
        lcd_write_char(*str++);
    }
}

void display_password(const char *password) {
    lcd_command(LCD_SETDDRAMADDR | 0x00); // Primera línea (0x00)
    lcd_print("Clave: ");
    lcd_print(password);
    
    // Calcular espacios restantes para limpiar basura anterior
    int len = 7 + strlen(password); // "Clave: " son 7 chars
    for (int i = len; i < 16; i++) {
        lcd_write_char(' ');
    }
}

void display_status(const char *status_line) {
    lcd_command(LCD_SETDDRAMADDR | 0x40); // Segunda línea (0x40)
    lcd_print(status_line);
    
    // Limpiar resto de la línea
    int len = strlen(status_line);
    for (int i = len; i < 16; i++) {
        lcd_write_char(' ');
    }
}