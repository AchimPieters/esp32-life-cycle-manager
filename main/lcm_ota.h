#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ota_url[256];
    char fallback_url[256];
    bool use_beta;
} ota_config_t;

void lcm_ota_start(void);
esp_err_t lcm_ota_get_config(ota_config_t *config);
esp_err_t lcm_ota_set_config(const ota_config_t *config);
esp_err_t lcm_ota_reset_config(void);

#ifdef __cplusplus
}
#endif
