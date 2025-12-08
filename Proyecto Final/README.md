# 📄 INFORME TÉCNICO: Ventilador Inteligente (ESP32)

## 1. 👥 Información General del Proyecto

| Campo | Detalle |
| :--- | :--- |
| **Título del Proyecto** | Ventilador Inteligente con Control MQTT y OTA |
| **Integrantes** | **Jhonatan Yara Lopez** |
| | **Edwin santiago Rodriguez Daza** |
| **Asignatura** | Estructuras Computacionales |
| **Plataforma** | ESP-IDF v5.5.1 (ESP32) |
| **Fecha de Entrega** | 8 de diciembre |

---

## 2. 🏛️ Arquitectura de Hardware

### 2.1. Componentes Físicos

El sistema está diseñado para el control domótico de un ventilador de bajo consumo, priorizando la conectividad remota y la capacidad de actualización.

* **Microcontrolador (MCU):** **ESP32** (Target: `esp32`). Seleccionado por su capacidad Wi-Fi integrada y el soporte nativo para FreeRTOS en el framework ESP-IDF.
* **Memoria Flash:** Módulo con **4MB** de memoria Flash (Requisito mínimo para el esquema de particiones OTA de doble aplicación).
* **Actuador de Velocidad:** Módulo de relé de estado sólido (SSR) o circuito basado en **PWM (Pulse Width Modulation)** para controlar la velocidad del motor del ventilador. Esto permite el control de velocidad en 4 niveles (0-3).
* **Fuente de Alimentación:** [Especificar, ej: Fuente conmutada 5V y regulador 3.3V]
* **Sensores (Opcional):** [Si se añaden, ej: Sensor DHTxx para lectura de temperatura y humedad, usado para el Modo `Auto`].

### 2.2. Diagrama de Conexiones
El siguiente diagrama representa la interconexión entre el ESP32 y el circuito de potencia para el control del ventilador.


---

## 3. 💾 Arquitectura de Firmware

El firmware se construyó utilizando el framework **ESP-IDF**, que se basa en el sistema operativo **FreeRTOS**, permitiendo una gestión concurrente de tareas críticas.

### 3.1. Patrones de Diseño Implementados

#### **A. Multi-tarea y Concurrencia (FreeRTOS)**
Se evita el patrón monolítico **Super Loop** tradicional, adoptando un enfoque basado en tareas de FreeRTOS para mejorar la robustez y la capacidad de respuesta (responsiveness).

* **`Wifi_Task`:** Gestiona el establecimiento y mantenimiento de la conexión de red.
* **`Mqtt_Task`:** Ejecuta el cliente MQTT, maneja la suscripción a comandos y la publicación de telemetría.
* **`Ventilador_Task`:** Contiene la lógica de control del dispositivo (Máquina de Estados), traduciendo el estado deseado (`fan_speed_state`) a acciones físicas (PWM/Relé).

La comunicación entre estas tareas se realiza mediante **EventGroups** para señalización de estado (ej. conexión) y potencialmente **Queues** para comandos complejos.

#### **B. Máquina de Estados para Conectividad**

Se implementa una Máquina de Estados (State Machine) para gestionar el ciclo de vida de la conexión de manera secuencial y robusta:

1.  **`STATE_DISCONNECTED`:** Intenta la reconexión WiFi hasta obtener IP.
2.  **`STATE_WIFI_CONNECTED`:** Inicia el cliente MQTT e intenta conectarse al broker.
3.  **`STATE_MQTT_CONNECTED`:** Modo operativo. Habilita la recepción de comandos y el reporte de telemetría, y se activa la `Ventilador_Task`.

### 3.2. Diagrama de Componentes de Firmware

El diagrama ilustra cómo las tareas se comunican a través de los mecanismos del sistema operativo FreeRTOS.



---

## 4. 💬 Protocolo de Comandos Remotos (MQTT)

La comunicación se basa en el broker MQTT, utilizando Quality of Service (QoS) 1 para comandos críticos que requieren confirmación de entrega.

| Tema (Topic) | Tipo de Mensaje | QoS | Retain | Función |
| :--- | :--- | :--- | :--- | :--- |
| `ventilador/control/velocidad` | `0` / `1` / `2` / `3` | 1 | No | Comando para establecer la velocidad del ventilador (0: OFF, 3: Máximo). |
| `ventilador/control/modo` | `Auto` / `Manual` | 1 | Sí | Comando para cambiar el modo de operación (si el ventilador es autónomo por temperatura). |
| `ventilador/status/velocidad` | `0-3` | 0 | No | Reporte periódico de la velocidad actual del motor. |
| `ventilador/telemetria/temperatura` | `float` | 0 | No | Publicación de la temperatura ambiente (si hay sensor). |

---

## 5. ⚡ Optimización Aplicada

La optimización fue fundamental, especialmente debido al requisito de implementar la funcionalidad de **Actualización Over-The-Air (OTA)**.

### 5.1. Gestión de Memoria Flash y Particiones
* **Particionado Personalizado:** Se utilizó el archivo `partitions_two_ota.csv` para definir dos particiones de aplicación grandes (`ota_0` y `ota_1`), cada una de **1984K**.
* **Ajuste de Flash:** Este esquema obligó a configurar el proyecto en `sdkconfig` para usar una memoria Flash de **4MB** (`CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y`), resolviendo el conflicto inicial con la configuración predeterminada de 2MB.

### 5.2. Optimización de Compilador y RAM
* **Reducción de Binario:** Se utilizó la bandera de compilación `-Os` (equivalente a `CONFIG_COMPILER_OPTIMIZATION_SIZE=y` en `menuconfig`) para priorizar el tamaño del binario generado, asegurando que el firmware cupiera en la partición de 1984K.
* **Ajuste Fino de FreeRTOS:** Se auditó el tamaño de pila (**Stack Size**) de las tareas de FreeRTOS, ajustándolo al valor mínimo seguro para conservar la memoria RAM (heap) dinámica y mejorar la estabilidad general del sistema.
