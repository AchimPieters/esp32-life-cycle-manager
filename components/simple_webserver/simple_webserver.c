#include "simple_webserver.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"

#define TAG "WEB"

static esp_err_t form_get_handler(httpd_req_t *req) {
    extern const unsigned char wifi_form_html_start[] asm("_binary_wifi_form_html_start");
    extern const unsigned char wifi_form_html_end[] asm("_binary_wifi_form_html_end");
    const size_t form_len = wifi_form_html_end - wifi_form_html_start;

    httpd_resp_send(req, (const char *)wifi_form_html_start, form_len);
    return ESP_OK;
}

static esp_err_t form_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));
    buf[ret] = '\0';

    char ssid[64], password[64];
    sscanf(buf, "ssid=%63[^&]&password=%63s", ssid, password);

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_disconnect();
    esp_wifi_connect();

    httpd_resp_send(req, "WiFi config saved. Rebooting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
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
