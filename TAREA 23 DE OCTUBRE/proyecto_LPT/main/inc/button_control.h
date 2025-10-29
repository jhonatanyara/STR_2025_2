// inc/button_control.h

#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

#include <stdbool.h> // Necesario para usar el tipo 'bool'

/**
 * @brief Inicializa el sistema de control (crea la tarea de monitoreo).
 */
void button_init(void);

/**
 * @brief Devuelve TRUE solo si se detectó una NUEVA pulsación del botón.
 * La bandera se limpia después de cada lectura.
 * @return true si el botón fue pulsado desde la última llamada, false en caso contrario.
 */
bool button_was_toggled(void);

#endif // BUTTON_CONTROL_H