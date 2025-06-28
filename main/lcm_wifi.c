#include "lcm_wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "simple_webserver.h"

#define TAG "LCM_WIFI"

void lcm_wifi_clear_config(void) {
    nvs_flash_erase();  // Wis volledige NVS
    esp_restart();
}

static bool wifi_config_exists(void) {
    wifi_config_t config;
    esp_wifi_get_config(WIFI_IF_STA, &config);
    return strlen((char*)config.sta.ssid) > 0;
}

void lcm_wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    if (wifi_config_exists()) {
        ESP_LOGI(TAG, "WiFi config found, connecting...");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
    } else {
        ESP_LOGW(TAG, "No WiFi config. Starting config portal...");
        esp_wifi_set_mode(WIFI_MODE_AP);
        wifi_config_t ap_config = {
            .ap = {
                .ssid = "ESP32-LCM",
                .ssid_len = 0,
                .channel = 1,
                .max_connection = 2,
                .authmode = WIFI_AUTH_OPEN
            }
        };
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        esp_wifi_start();
        simple_webserver_start();  // Start HTTP server
    }
}
