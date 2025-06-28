#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lcm_bootcount.h"
#include "lcm_wifi.h"
#include "lcm_ota.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Life-Cycle-Manager starting...");

    // Initialiseer NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Bootcount logic
    lcm_bootcount_init();
    uint32_t bootcount = lcm_get_bootcount();
    ESP_LOGI(TAG, "Bootcount = %u", bootcount);

    // Test-config (optioneel verwijderen in productie)
    ota_config_t test_cfg = { .use_beta = false };
    strcpy(test_cfg.ota_url,      CONFIG_OTA_FIRMWARE_URL);
    strcpy(test_cfg.fallback_url, "");  // lege fallback standaard
    lcm_ota_set_config(&test_cfg);

    // Lifecycle acties
    if (bootcount <= 4) {
        ESP_LOGI(TAG, "Normal boot.");
    } else if (bootcount <= 7) {
        ESP_LOGW(TAG, "Bootcount = %u → Trigger OTA update", bootcount);
        lcm_ota_start();
        lcm_clear_bootcount();
    } else if (bootcount <= 10) {
        ESP_LOGW(TAG, "Clearing WiFi config...");
        lcm_wifi_clear_config();
        lcm_clear_bootcount();
    } else if (bootcount <= 13) {
        ESP_LOGW(TAG, "Activating beta mode...");
        ota_config_t tmp = {0};
        lcm_ota_get_config(&tmp);
        tmp.use_beta = true;
        lcm_ota_set_config(&tmp);
        lcm_wifi_clear_config();
        lcm_clear_bootcount();
    } else {
        ESP_LOGE(TAG, "FACTORY RESET");
        nvs_flash_erase();
        esp_restart();
    }

    // Start WiFi (AP of STA + webserver)
    lcm_wifi_init();

    // Hoofdloop
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Running...");
    }
}
