#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * @brief Start de configuratie-webserver (SoftAP moet al actief zijn).
 */
void simple_webserver_start(void);

#ifdef __cplusplus
}
#endif
