#include "Display.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OLED";
static i2c_master_dev_handle_t dev_handle;
static bool display_ok = false;

#define I2C_TIMEOUT_MS 50 
#define SH1106_OFFSET 0x02 

// Fuente 5x7 (Resumida para ahorrar espacio, mantén tu array completo si lo tienes localmente)
// Asegúrate de que este array coincida con el que ya tenías funcionando.
const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x07, 0x00, 0x07, 0x00, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z    
};

static void send_cmd(uint8_t cmd) {
    if (!display_ok) return;
    uint8_t data[] = {0x00, cmd};
    i2c_master_transmit(dev_handle, data, sizeof(data), I2C_TIMEOUT_MS);
}

static void send_data(uint8_t *data, size_t len) {
    if (!display_ok) return;
    uint8_t *buf = malloc(len + 1);
    if (!buf) return;
    buf[0] = 0x40; 
    memcpy(buf + 1, data, len);
    i2c_master_transmit(dev_handle, buf, len + 1, I2C_TIMEOUT_MS);
    free(buf);
}

static void print_line(int page, const char *str) {
    if (!display_ok) return;
    
    send_cmd(0xB0 + page); 
    send_cmd(0x00 + SH1106_OFFSET); 
    send_cmd(0x10); 
    
    uint8_t buffer[128] = {0};
    int idx = 0;
    while (*str && idx < 127) {
        char c = *str++;
        int f_idx = 0;
        
        if (c >= 32 && c <= 90) {
            f_idx = c - 32;
        } else if (c >= 'a' && c <= 'z') {
            f_idx = (c - 32) - 32; 
        } else {
            f_idx = 0; 
        }
        
        for (int i=0; i<5; i++) if (idx < 128) buffer[idx++] = font5x7[f_idx][i];
        if (idx < 128) buffer[idx++] = 0x00; 
    }
    send_data(buffer, 128);
}

void display_init(void) {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (err == ESP_OK) {
        display_ok = true;
        uint8_t init_cmds[] = {
            0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
            0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
            0xDB, 0x40, 0xA4, 0xA6, 0xAF
        };
        for (int i = 0; i < sizeof(init_cmds); i++) send_cmd(init_cmds[i]);
        
        // Limpiar pantalla
        for (int i = 0; i < 8; i++) print_line(i, "                ");
        
        ESP_LOGI(TAG, "OLED SH1106 Inicializada Correctamente.");
    } else {
        ESP_LOGE(TAG, "Error al encontrar la pantalla OLED.");
        display_ok = false;
    }
}

// FUNCION ACTUALIZADA: AHORA CREA MÁSCARA DE ASTERISCOS
void display_update_ui(const char *status, const char *password, int motor_percent, float temp) {
    if (!display_ok) return;

    char buffer[32];

    // Línea 0: Estado
    snprintf(buffer, sizeof(buffer), "EST: %s", status);
    print_line(0, buffer);

    // Línea 2: PASSWORD CON ASTERISCOS
    // Creamos un string con tantos asteriscos como caracteres tenga el password
    char masked_pass[10] = "";
    int pass_len = strlen(password);
    if (pass_len > 8) pass_len = 8; // Límite de seguridad visual
    
    for(int i=0; i<pass_len; i++) {
        masked_pass[i] = '*';
    }
    masked_pass[pass_len] = '\0'; // Terminar string

    snprintf(buffer, sizeof(buffer), "PASS: %s", masked_pass);
    print_line(2, buffer);

    // Línea 4: Motor y Temperatura
    snprintf(buffer, sizeof(buffer), "FAN:%d%% %.1fC", motor_percent, temp);
    print_line(4, buffer);
    
    // Limpieza estética
    print_line(1, "                ");
    print_line(3, "                ");
    print_line(5, "                ");
    print_line(6, "                ");
    print_line(7, "                ");
}