#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "termistor.h"

void app_main(void)
{
    termistor_init();

    while (1) {
        int   raw = termistor_read_raw();
        float mv  = termistor_read_millivolts();
        float tc  = termistor_read_celsius();
        printf("NTC raw=%4d  V=%.3f  T=%.2f °C\n", raw, mv/1000.0f, tc);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
