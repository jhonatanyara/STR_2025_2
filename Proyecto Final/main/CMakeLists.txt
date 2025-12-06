idf_component_register(
    SRCS 
        "SRC/main.c" 
        "SRC/Keypad.c"
        "SRC/Motor.c"   # <--- AGREGADO
        "SRC/Sensor.c" # <--- AGREGADO
    INCLUDE_DIRS 
        "SRC"
    REQUIRES
        "esp_netif"
        "esp_wifi"
        "nvs_flash"
        "esp_driver_gpio"
        "esp_driver_ledc" # <--- IMPORTANTE: Necesario para el PWM
        "log"
        "freertos"
)