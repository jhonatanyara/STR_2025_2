/*
 * http_server.h
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#include "esp_http_server.h"

// Inicia el servidor web
httpd_handle_t start_webserver(void);

// Detiene el servidor web
void stop_webserver(httpd_handle_t server);

#endif /* MAIN_HTTP_SERVER_H_ */