#include "lcm_ota.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

static const char *TAG = "LCM_OTA";

// Statische firmware URL (kan later uit NVS komen)
#define OTA_URL CONFIG_OTA_FIRMWARE_URL

extern const uint8_t server_cert_pem_start[] asm("_binary_certs_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_certs_pem_end");

void lcm_ota_start(void) {
    ESP_LOGI(TAG, "Starting OTA update from: %s", OTA_URL);

    esp_http_client_config_t config = {
        .url = OTA_URL,
        .cert_pem = (char *)server_cert_pem_start,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful, restarting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
    }
}
