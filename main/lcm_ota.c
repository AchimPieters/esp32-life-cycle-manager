#include "lcm_ota.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define TAG "LCM_OTA"
#define OTA_NVS_NAMESPACE "ota_cfg"

extern const uint8_t server_cert_pem_start[] asm("_binary_certs_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_certs_pem_end");

esp_err_t lcm_ota_get_config(ota_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;

    size_t len = sizeof(config->ota_url);
    err |= nvs_get_str(nvs, "url", config->ota_url, &len);

    len = sizeof(config->fallback_url);
    err |= nvs_get_str(nvs, "fallback", config->fallback_url, &len);

    uint8_t beta = 0;
    err |= nvs_get_u8(nvs, "beta", &beta);
    config->use_beta = beta;

    nvs_close(nvs);
    return err;
}

esp_err_t lcm_ota_set_config(const ota_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    err |= nvs_set_str(nvs, "url", config->ota_url);
    err |= nvs_set_str(nvs, "fallback", config->fallback_url);
    err |= nvs_set_u8(nvs, "beta", config->use_beta);
    err |= nvs_commit(nvs);

    nvs_close(nvs);
    return err;
}

esp_err_t lcm_ota_reset_config(void) {
    return nvs_erase_namespace(OTA_NVS_NAMESPACE);
}

static esp_err_t try_ota_update(const char *url) {
    ESP_LOGI(TAG, "Attempting OTA from: %s", url);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 10000,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA successful.");
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

void lcm_ota_start(void) {
    ota_config_t config = {0};

    if (lcm_ota_get_config(&config) != ESP_OK || strlen(config.ota_url) == 0) {
        ESP_LOGW(TAG, "Using fallback built-in OTA URL.");
        strncpy(config.ota_url, CONFIG_OTA_FIRMWARE_URL, sizeof(config.ota_url));
    }

    const char *url = config.ota_url;
    if (config.use_beta && strlen(config.fallback_url) > 0) {
        ESP_LOGI(TAG, "Beta mode active: prioritizing fallback OTA.");
        url = config.fallback_url;
    }

    esp_err_t ret = try_ota_update(url);
    if (ret == ESP_OK) {
        esp_restart();
    }

    // Probeer fallback als deze nog niet geprobeerd werd
    if (strcmp(url, config.fallback_url) != 0 && strlen(config.fallback_url) > 0) {
        ESP_LOGW(TAG, "Primary OTA failed. Trying fallback...");
        ret = try_ota_update(config.fallback_url);
        if (ret == ESP_OK) {
            esp_restart();
        }
    }

    ESP_LOGE(TAG, "All OTA attempts failed.");
}
