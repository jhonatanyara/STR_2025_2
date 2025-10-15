idf_component_register(
  SRCS
    "app_main.c"
    "termistor.c"
  INCLUDE_DIRS "inc"
  REQUIRES driver esp_adc        # si tu IDF no trae esp_adc, deja solo: REQUIRES driver
)

