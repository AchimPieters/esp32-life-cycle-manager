#include "simple_webserver.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "lcm_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WEB";

/**
 * GET handler: scan Wi-Fi, bouw dynamisch HTML-formulier.
 */
static esp_err_t form_get_handler(httpd_req_t *req)
{
    // 1) Scan starten (blokkerend)
    wifi_scan_config_t scan_conf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };
    ESP_ERROR_CHECK( esp_wifi_scan_start(&scan_conf, true) );

    // 2) Resultaten ophalen
    uint16_t ap_count = 20;
    wifi_ap_record_t ap_records[20];
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records(&ap_count, ap_records) );

    // 3) Begin HTML
    const char *hdr =
        "<!DOCTYPE html><html><head><title>ESP32 Config</title></head><body>"
        "<h2>Configure Wi-Fi & OTA</h2>"
        "<form method=\"POST\" action=\"/submit\">"
        "<label>SSID:</label><br>"
        "<select name=\"ssid\">";
    size_t buf_size = 4096 + ap_count * 128;
    char *html = malloc(buf_size);
    if (!html) {
        ESP_LOGE(TAG, "Malloc failed");
        return ESP_FAIL;
    }
    html[0] = 0;
    strncat(html, hdr, buf_size - strlen(html) - 1);

    // 4) Voeg elke netwerk toe
    for (int i = 0; i < ap_count; i++) {
        char line[128];
        const char *lock = (ap_records[i].authmode == WIFI_AUTH_OPEN) ? "🔓" : "🔒";
        snprintf(line, sizeof(line),
            "<option value=\"%s\">%s %s (%ddBm)</option>",
            ap_records[i].ssid, lock, ap_records[i].ssid, ap_records[i].rssi);
        strncat(html, line, buf_size - strlen(html) - 1);
    }
    strncat(html,
        "</select><br><br>"
        "<label>Password:</label><br>"
        "<input type=\"password\" name=\"password\"><br><br>"
        "<label>OTA URL:</label><br>"
        "<input type=\"text\" name=\"ota_url\" size=\"50\" value=\"\"><br><br>"
        "<label>Fallback URL:</label><br>"
        "<input type=\"text\" name=\"fallback_url\" size=\"50\" value=\"\"><br><br>"
        "<label><input type=\"checkbox\" name=\"use_beta\"> Use Beta Firmware</label><br><br>"
        "<input type=\"submit\" value=\"Save & Reboot\">"
        "</form></body></html>",
        buf_size - strlen(html) - 1);

    // 5) Response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    free(html);
    return ESP_OK;
}

/**
 * POST handler: lees body, parseer velden, zet wifi & OTA, reboot.
 */
static esp_err_t form_post_handler(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content");
        return ESP_FAIL;
    }

    char *buf = malloc(total + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    int ret = httpd_req_recv(req, buf, total);
    buf[ret] = '\0';

    // Parse URL-encoded body
    char ssid[64]          = {0};
    char password[64]      = {0};
    char ota_url[256]      = {0};
    char fallback_url[256] = {0};
    bool use_beta = false;

    sscanf(buf,
        "ssid=%63[^&]&password=%63[^&]&ota_url=%255[^&]&fallback_url=%255[^&]",
        ssid, password, ota_url, fallback_url);
    if (strstr(buf, "use_beta=on")) {
        use_beta = true;
    }
    free(buf);

    ESP_LOGI(TAG, "Configuring WiFi SSID='%s', OTA='%s', beta=%d",
             ssid, ota_url, use_beta);

    // WiFi verbinden
    wifi_config_t wcfg = { 0 };
    strncpy((char*)wcfg.sta.ssid, ssid, sizeof(wcfg.sta.ssid));
    strncpy((char*)wcfg.sta.password, password, sizeof(wcfg.sta.password));
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    esp_wifi_disconnect();
    esp_wifi_connect();

    // OTA config opslaan
    ota_config_t o_cfg = { 0 };
    strncpy(o_cfg.ota_url,      ota_url,      sizeof(o_cfg.ota_url));
    strncpy(o_cfg.fallback_url, fallback_url, sizeof(o_cfg.fallback_url));
    o_cfg.use_beta = use_beta;
    lcm_ota_set_config(&o_cfg);

    // Ack & reboot
    const char *resp = "Configuration saved, rebooting...";
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, resp, strlen(resp));
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
    return ESP_OK;
}

/**
 * Start HTTP-server met GET/POST routes voor config portal.
 */
void simple_webserver_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK( httpd_start(&server, &config) );

    httpd_uri_t get_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = form_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_uri);

    httpd_uri_t post_uri = {
        .uri      = "/submit",
        .method   = HTTP_POST,
        .handler  = form_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &post_uri);

    ESP_LOGI(TAG, "Config portal running");
}
