# proyecto_LPT/main/CMakeLists.txt
idf_component_register(
    SRCS "adc_control.c" "main.c"
         "uart_cmd.c"
         "button_control.c"
         "termistor.c"
         "led_pwm.c"
         "potenciometro.c"
    INCLUDE_DIRS "inc"   # si tus .h están en main/inc
    REQUIRES driver freertos esp_timer esp_adc
)
