#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcm_bootcount.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // Bootcount init
    lcm_bootcount_init();
    uint32_t bootcount = lcm_get_bootcount();

    ESP_LOGI(TAG, "Bootcount = %u", bootcount);

    // Lifecycle acties gebaseerd op bootcount
    if (bootcount <= 4) {
        ESP_LOGI(TAG, "Normal boot");
    } else if (bootcount <= 7) {
        ESP_LOGW(TAG, "Trigger OTA update...");
        // lcm_ota_start();  <-- straks toevoegen
        lcm_clear_bootcount();
    } else if (bootcount <= 10) {
        ESP_LOGW(TAG, "Wiping WiFi and config");
        // lcm_wifi_clear_config(); <-- later
        lcm_clear_bootcount();
    } else if (bootcount <= 13) {
        ESP_LOGE(TAG, "Beta mode activated + wipe WiFi");
        // lcm_wifi_clear_config();
        // lcm_enable_beta_mode(); <-- NVS flag
        lcm_clear_bootcount();
    } else {
        ESP_LOGE(TAG, "FACTORY RESET initiated!");
        // lcm_factory_reset(); <-- wis NVS
        lcm_clear_bootcount();
    }

    // Rest van applicatie
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Running...");
    }
}
