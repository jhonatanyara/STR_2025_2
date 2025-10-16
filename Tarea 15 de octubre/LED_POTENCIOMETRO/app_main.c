#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "inc/potenciometro.h"
#include "inc/led_pwm.h"
#include "driver/ledc.h"


void app_main(void)
{
    potenciometro_init();
    led_pwm_init();

    while (1) {
        int pct = potenciometro_porcentaje_0_100();
        led_pwm_set_pct(pct);

        // (Opcional) info por consola
        int raw = potenciometro_leer_crudo_prom(8);
        int mv  = potenciometro_leer_milivoltios();
        printf("POT raw=%4d  mv=%4d  brillo=%3d %%\n", raw, mv, pct);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
