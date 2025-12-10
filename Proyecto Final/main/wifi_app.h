/*
 * wifi_app.h - Versión Limpia
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"

// -----------------------------------------------------
// 1. CONFIGURACIÓN DEL PUNTO DE ACCESO (AP)
// -----------------------------------------------------
// Puedes cambiar estos valores si quieres otro nombre de WiFi
#define WIFI_AP_SSID                "ESP32_VENTILADOR"  
#define WIFI_AP_PASSWORD            "12345678"          
#define WIFI_AP_CHANNEL             1                   
#define WIFI_AP_SSID_HIDDEN         0                   
#define WIFI_AP_MAX_CONNECTIONS     5                   
#define WIFI_AP_BEACON_INTERVAL     100                 
#define WIFI_AP_IP                  "192.168.0.1"       
#define WIFI_AP_GATEWAY             "192.168.0.1"       
#define WIFI_AP_NETMASK             "255.255.255.0"     
#define WIFI_AP_BANDWIDTH           WIFI_BW_HT20        
#define WIFI_STA_POWER_SAVE         WIFI_PS_NONE        
#define MAX_SSID_LENGTH             32                  
#define MAX_PASSWORD_LENGTH         64                  
#define MAX_CONNECTION_RETRIES      5                   

// Objetos de red (declaración externa)
extern esp_netif_t* esp_netif_sta;
extern esp_netif_t* esp_netif_ap;

// -----------------------------------------------------
// 2. MENSAJES Y COLAS
// -----------------------------------------------------
/**
 * IDs de mensajes para la tarea WiFi
 */
typedef enum wifi_app_message
{
    WIFI_APP_MSG_START_HTTP_SERVER = 0,
    WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP,
    WIFI_APP_MSG_STA_DISCONNECTED,
    WIFI_APP_CONNECT_TO_STA,
} wifi_app_message_e;

/**
 * Estructura de la cola de mensajes
 */
typedef struct wifi_app_queue_message
{
    wifi_app_message_e msgID;
} wifi_app_queue_message_t;


// -----------------------------------------------------
// 3. FUNCIONES PÚBLICAS
// -----------------------------------------------------

/**
 * Inicia la tarea principal del WiFi (Llamar desde main.c)
 */
void wifi_app_start(void);

/**
 * Envía un mensaje a la cola del WiFi
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/**
 * Obtiene la configuración actual del WiFi
 */
wifi_config_t* wifi_app_get_wifi_config(void);

/**
 * Inicializa/Fuerza la sincronización de hora NTP
 */
void init_obtain_time(void);

#endif /* MAIN_WIFI_APP_H_ */