#include "lcm_ota.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_system.h"
#include "nvs.h"
#include <string.h>

#define TAG "LCM_OTA"
#define OTA_NVS_NAMESPACE "ota_cfg"

extern const uint8_t server_cert_pem_start[] asm("_binary_certs_pem_start");

static esp_err_t try_ota_update(const char *url) {
    ESP_LOGI(TAG, "Attempting OTA from: %s", url);
    esp_http_client_config_t http_cfg = {
        .url = url,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 10000,
    };
    esp_https_ota_config_t ota_cfg = { .http_config = &http_cfg };
    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA successful.");
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t lcm_ota_get_config(ota_config_t *config) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t len;
    len = sizeof(config->ota_url);
    err |= nvs_get_str(h, "url", config->ota_url, &len);
    len = sizeof(config->fallback_url);
    err |= nvs_get_str(h, "fallback", config->fallback_url, &len);
    uint8_t beta = 0;
    err |= nvs_get_u8(h, "beta", &beta);
    config->use_beta = beta;
    nvs_close(h);
    return err;
}

esp_err_t lcm_ota_set_config(const ota_config_t *config) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err |= nvs_set_str(h, "url", config->ota_url);
    err |= nvs_set_str(h, "fallback", config->fallback_url);
    err |= nvs_set_u8(h, "beta", config->use_beta);
    err |= nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t lcm_ota_reset_config(void) {
    return nvs_erase_namespace(OTA_NVS_NAMESPACE);
}

void lcm_ota_start(void) {
    ota_config_t cfg = {0};
    if (lcm_ota_get_config(&cfg) != ESP_OK || strlen(cfg.ota_url) == 0) {
        ESP_LOGW(TAG, "No valid NVS config, using default.");
        strncpy(cfg.ota_url, CONFIG_OTA_FIRMWARE_URL, sizeof(cfg.ota_url));
    }
    const char *url = cfg.ota_url;
    if (cfg.use_beta && strlen(cfg.fallback_url) > 0) {
        ESP_LOGI(TAG, "Beta active, prioritizing fallback URL.");
        url = cfg.fallback_url;
    }
    esp_err_t r = try_ota_update(url);
    if (r == ESP_OK) esp_restart();
    if (strcmp(url, cfg.fallback_url) != 0 && strlen(cfg.fallback_url) > 0) {
        ESP_LOGW(TAG, "Primary failed, trying fallback...");
        r = try_ota_update(cfg.fallback_url);
        if (r == ESP_OK) esp_restart();
    }
    ESP_LOGE(TAG, "All OTA sources failed.");
}
