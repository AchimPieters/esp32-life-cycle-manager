#include "nvs_flash.h"
#include "nvs.h"

#define OTA_NVS_NAMESPACE "ota_cfg"

esp_err_t lcm_ota_get_config(ota_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;

    size_t url_len = sizeof(config->ota_url);
    err |= nvs_get_str(nvs, "url", config->ota_url, &url_len);

    size_t fbu_len = sizeof(config->fallback_url);
    err |= nvs_get_str(nvs, "fallback", config->fallback_url, &fbu_len);

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
    return nvs_erase_namespace(OTA_NVS_NAMESPACE);  // Wis alles
}
