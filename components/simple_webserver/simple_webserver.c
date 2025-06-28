#include "simple_webserver.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "lcm_ota.h"

#define TAG "WEB"

static esp_err_t form_get_handler(httpd_req_t *req) {
        extern const unsigned char wifi_form_html_start[] asm ("_binary_wifi_form_html_start");
        extern const unsigned char wifi_form_html_end[] asm ("_binary_wifi_form_html_end");
        const size_t form_len = wifi_form_html_end - wifi_form_html_start;

        httpd_resp_send(req, (const char *)wifi_form_html_start, form_len);
        return ESP_OK;
}

static esp_err_t form_post_handler(httpd_req_t *req) {
        char ssid[64] = {0}, password[64] = {0};
        char ota_url[256] = {0}, fallback_url[256] = {0};
        bool use_beta = false;

        sscanf(buf, "ssid=%63[^&]&password=%63[^&]&ota_url=%255[^&]&fallback_url=%255[^&]",
               ssid, password, ota_url, fallback_url);

// Checkbox (optioneel, dus we zoeken handmatig)
        if (strstr(buf, "use_beta=on")) {
                use_beta = true;
        }

// WiFi config
        wifi_config_t wifi_config = {0};
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

// OTA config opslaan in NVS
        ota_config_t ota_config = {0};
        strncpy(ota_config.ota_url, ota_url, sizeof(ota_config.ota_url));
        strncpy(ota_config.fallback_url, fallback_url, sizeof(ota_config.fallback_url));
        ota_config.use_beta = use_beta;
        lcm_ota_set_config(&ota_config);

        httpd_resp_send(req, "Config saved. Rebooting...", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(pdMS_TO_TICKS(1500));
        esp_restart();
}

void simple_webserver_start(void) {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        httpd_handle_t server = NULL;
        httpd_start(&server, &config);

        httpd_uri_t get_uri = {
                .uri = "/",
                .method = HTTP_GET,
                .handler = form_get_handler,
        };
        httpd_uri_t post_uri = {
                .uri = "/submit",
                .method = HTTP_POST,
                .handler = form_post_handler,
        };
        httpd_register_uri_handler(server, &get_uri);
        httpd_register_uri_handler(server, &post_uri);
}
