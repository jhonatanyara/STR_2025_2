idf_component_register(
    SRCS "app_main.c" "potenciometro.c"
    INCLUDE_DIRS "inc"
    REQUIRES driver esp_adc
)
