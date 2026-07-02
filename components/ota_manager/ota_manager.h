
#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ota_init(void);

/**
 * Perform an HTTPS OTA update.
 *
 * The OTA manager validates the HTTPS response, checks the incoming app
 * descriptor before flashing the full image, skips same-version updates, and
 * lets esp_https_ota validate the final image before switching boot partition.
 *
 * url must use https:// and have a trusted server certificate.
 * Returns ESP_OK when the update succeeds or when the same version is skipped.
 */
esp_err_t ota_update(const char *url);

#ifdef __cplusplus
}
#endif

#endif
