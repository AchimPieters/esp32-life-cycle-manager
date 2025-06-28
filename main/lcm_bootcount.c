#include "lcm_bootcount.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_reset_reason.h"
#include "esp_attr.h"

#define TAG "LCM_BOOTCOUNT"
#define MAX_RESET_TIME_MS 8000   // Max tijd tussen herstarts om te tellen als kort
#define BOOTCOUNT_MAGIC 0xC0DE1234

typedef struct {
        uint32_t magic;
        uint32_t count;
        uint64_t last_reset_time;
} rtc_boot_data_t;

RTC_DATA_ATTR static rtc_boot_data_t rtc_data;

void lcm_bootcount_init(void) {

        if (esp_reset_reason() == ESP_RST_SW) {
                ESP_LOGI(TAG, "Software reset — bootcount not incremented.");
                return;
        }

        uint64_t now = esp_timer_get_time() / 1000; // ms

        if (rtc_data.magic != BOOTCOUNT_MAGIC) {
                rtc_data.magic = BOOTCOUNT_MAGIC;
                rtc_data.count = 1;
                rtc_data.last_reset_time = now;
                ESP_LOGI(TAG, "First boot after power cycle.");
        } else {
                uint64_t delta = now - rtc_data.last_reset_time;
                if (delta < MAX_RESET_TIME_MS) {
                        rtc_data.count++;
                } else {
                        rtc_data.count = 1;
                }
                rtc_data.last_reset_time = now;
                ESP_LOGI(TAG, "Reboot detected. Bootcount: %u", rtc_data.count);
        }
}

uint32_t lcm_get_bootcount(void) {
        return rtc_data.count;
}

void lcm_clear_bootcount(void) {
        rtc_data.magic = 0;
        rtc_data.count = 0;
        rtc_data.last_reset_time = 0;
}
