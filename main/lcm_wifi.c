#include "lcm_wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "simple_webserver.h"
#include <string.h>

#define TAG "LCM_WIFI"

static bool wifi_config_exists(void) {
    wifi_config_t cfg;
    esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &cfg);
    return (err == ESP_OK && strlen((char*)cfg.sta.ssid) > 0);
}

void lcm_wifi_clear_config(void) {
    nvs_flash_erase();
    esp_restart();
}

void lcm_wifi_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&init_cfg);

    if (wifi_config_exists()) {
        ESP_LOGI(TAG, "Connecting to saved WiFi...");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
    } else {
        ESP_LOGW(TAG, "No WiFi config found, starting AP + config portal...");
        wifi_config_t ap = {
            .ap = {
                .ssid = "ESP32-LCM",
                .ssid_len = 0,
                .authmode = WIFI_AUTH_OPEN,
                .max_connection = 4,
                .channel = 1
            }
        };
        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_config(WIFI_IF_AP, &ap);
        esp_wifi_start();
        simple_webserver_start();
    }
}
