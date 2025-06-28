#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcm_bootcount.h"
#include "lcm_wifi.h"
#include "lcm_ota.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Life-Cycle-Manager starting...");

    lcm_bootcount_init();
    uint32_t bootcount = lcm_get_bootcount();
    ESP_LOGI(TAG, "Bootcount = %u", bootcount);

    // Voor testdoeleinden kun je OTA config forceren (verwijder straks)
    ota_config_t cfg = {
        .use_beta = false
    };
    strcpy(cfg.ota_url, "https://github.com/AchimPieters/esp32-life-cycle-manager/releases/latest/download/firmware.bin");
    strcpy(cfg.fallback_url, "https://fallback.server.com/firmware.bin");
    lcm_ota_set_config(&cfg);

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
        ota_config_t cfg = {0};
        lcm_ota_get_config(&cfg);
        cfg.use_beta = true;
        lcm_ota_set_config(&cfg);
        lcm_wifi_clear_config();
        lcm_clear_bootcount();
    } else {
        ESP_LOGE(TAG, "FACTORY RESET");
        nvs_flash_erase();
        esp_restart();
    }

    lcm_wifi_init();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Running...");
    }
}
