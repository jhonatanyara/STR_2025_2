#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include <sys/param.h>

static const char *TAG = "HTTP_SERVER";

// 1. REFERENCIAS EXTERNAS
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

typedef struct {
    bool active;
    int start_hour;
    int end_hour;
    float t_zero;
    float t_hundred;
} schedule_t;

extern int system_mode;
extern int manual_pwm_val;
extern float current_temp;
extern bool pir_state;
extern int current_pwm_output;
extern float auto_tmin;
extern float auto_tmax;
extern schedule_t schedules[3]; 

// 2. FUNCIONES NVS
esp_err_t save_int_to_nvs(const char *key, int value) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_i32(my_handle, key, value);
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
    return err;
}

// 3. HANDLER OTA
static esp_err_t ota_update_post_handler(httpd_req_t *req)
{
    char buf[1024];
    esp_ota_handle_t ota_handle;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    
    if (update_partition == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Iniciando OTA...");
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int remaining = req->content_len;
    while (remaining > 0) {
        int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (recv_len <= 0) {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) continue;
            esp_ota_end(ota_handle);
            return ESP_FAIL;
        }
        esp_ota_write(ota_handle, buf, recv_len);
        remaining -= recv_len;
    }

    if (esp_ota_end(ota_handle) == ESP_OK) {
        esp_ota_set_boot_partition(update_partition);
        httpd_resp_send(req, "OTA OK", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    return ESP_OK;
}

// 4. HANDLERS WEB
static esp_err_t webpage_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temp", current_temp);
    cJSON_AddBoolToObject(root, "pir", pir_state);
    cJSON_AddNumberToObject(root, "pwm", current_pwm_output);
    cJSON_AddNumberToObject(root, "mode", system_mode);
    cJSON_AddNumberToObject(root, "man_pwm", manual_pwm_val);
    cJSON_AddNumberToObject(root, "a_min", auto_tmin);
    cJSON_AddNumberToObject(root, "a_max", auto_tmax);

    cJSON *schedArray = cJSON_CreateArray();
    for(int i=0; i<3; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddBoolToObject(item, "act", schedules[i].active);
        cJSON_AddNumberToObject(item, "sh", schedules[i].start_hour);
        cJSON_AddNumberToObject(item, "eh", schedules[i].end_hour);
        cJSON_AddNumberToObject(item, "t0", schedules[i].t_zero);
        cJSON_AddNumberToObject(item, "t100", schedules[i].t_hundred);
        cJSON_AddItemToArray(schedArray, item);
    }
    cJSON_AddItemToObject(root, "schedules", schedArray);

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t settings_post_handler(httpd_req_t *req) {
    char buf[2048]; 
    int remaining = req->content_len;
    if (remaining >= sizeof(buf)) { httpd_resp_send_500(req); return ESP_FAIL; }
    
    int received = 0;
    while (remaining > 0) {
        int ret = httpd_req_recv(req, buf + received, remaining);
        if (ret <= 0) { 
             if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
             return ESP_FAIL; 
        }
        received += ret;
        remaining -= ret;
    }
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) return ESP_FAIL;

    // Guardar configuraciones simples
    cJSON *item = cJSON_GetObjectItem(root, "mode");
    if (item) { system_mode = item->valueint; save_int_to_nvs("sys_mode", system_mode); }
    
    item = cJSON_GetObjectItem(root, "manual_pwm");
    if (item) { manual_pwm_val = item->valueint; save_int_to_nvs("man_pwm", manual_pwm_val); }

    item = cJSON_GetObjectItem(root, "auto_tmin");
    if (item) { auto_tmin = (float)item->valuedouble; save_int_to_nvs("auto_tmin", (int)(auto_tmin * 100)); }

    item = cJSON_GetObjectItem(root, "auto_tmax");
    if (item) { auto_tmax = (float)item->valuedouble; save_int_to_nvs("auto_tmax", (int)(auto_tmax * 100)); }

    // Guardar Schedules
    cJSON *schedArr = cJSON_GetObjectItem(root, "schedules");
    if (schedArr && cJSON_IsArray(schedArr)) {
        for (int i = 0; i < 3; i++) {
            cJSON *s = cJSON_GetArrayItem(schedArr, i);
            if (s) {
                schedules[i].active = cJSON_GetObjectItem(s, "act")->valueint;
                schedules[i].start_hour = cJSON_GetObjectItem(s, "sh")->valueint;
                schedules[i].end_hour = cJSON_GetObjectItem(s, "eh")->valueint;
                schedules[i].t_zero = (float)cJSON_GetObjectItem(s, "t0")->valuedouble;
                schedules[i].t_hundred = (float)cJSON_GetObjectItem(s, "t100")->valuedouble;

                char key[16];
                sprintf(key, "sch%d_act", i); save_int_to_nvs(key, schedules[i].active);
                sprintf(key, "sch%d_sh", i);  save_int_to_nvs(key, schedules[i].start_hour);
                sprintf(key, "sch%d_eh", i);  save_int_to_nvs(key, schedules[i].end_hour);
                sprintf(key, "sch%d_t0", i);  save_int_to_nvs(key, (int)(schedules[i].t_zero * 100));
                sprintf(key, "sch%d_t1", i);  save_int_to_nvs(key, (int)(schedules[i].t_hundred * 100));
            }
        }
    }

    cJSON_Delete(root);
    httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 5. INICIO DEL SERVIDOR
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192; 
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = webpage_get_handler };
        httpd_register_uri_handler(server, &uri_root);

        httpd_uri_t uri_status = { .uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler };
        httpd_register_uri_handler(server, &uri_status);

        httpd_uri_t uri_settings = { .uri = "/api/settings", .method = HTTP_POST, .handler = settings_post_handler };
        httpd_register_uri_handler(server, &uri_settings);

        httpd_uri_t uri_ota = { .uri = "/ota", .method = HTTP_POST, .handler = ota_update_post_handler };
        httpd_register_uri_handler(server, &uri_ota);

        ESP_LOGI(TAG, "Web Server + OTA iniciado");
        return server;
    }
    return NULL;
}

void stop_webserver(httpd_handle_t server) {
    if (server) httpd_stop(server);
}