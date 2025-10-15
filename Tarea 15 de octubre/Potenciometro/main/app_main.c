#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "potenciometro.h"

void app_main(void)
{
    potenciometro_init();

    while (1) {
        int crudo = potenciometro_leer_crudo_prom(8);
        int mv    = potenciometro_leer_milivoltios();
        int pct   = potenciometro_porcentaje_0_100();

        printf("POT crudo=%4d  V=%.3f V  pct=%3d%%\n", crudo, mv/1000.0f, pct);

        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz
    }
}
