#include <stdbool.h>
#include <string.h>

#include "ota_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_app_desc.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif

#ifdef OTA_SERVER_CERT_EMBEDDED
extern const char ota_server_cert_pem_start[] asm("_binary_ota_server_cert_pem_start");
#endif

static const char *TAG = "OTA_SIMPLE";

#define OTA_PROGRESS_STEP_BYTES (50 * 1024)

static esp_err_t validate_image_header(const esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (new_app_info->magic_word != ESP_APP_DESC_MAGIC_WORD) {
        ESP_LOGE(TAG, "Invalid app descriptor magic word: 0x%lx", new_app_info->magic_word);
        return ESP_ERR_INVALID_RESPONSE;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info = {0};
    esp_err_t err = esp_ota_get_partition_description(running, &running_app_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read running app description: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "  Current version: %s", running_app_info.version);
    ESP_LOGI(TAG, "  New version:     %s", new_app_info->version);

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "New image version is the same as the running image; skipping OTA");
        return ESP_ERR_INVALID_VERSION;
    }

    return ESP_OK;
}

static bool ota_url_uses_https(const char *url)
{
    return url != NULL && strncmp(url, "https://", strlen("https://")) == 0;
}

esp_err_t ota_init(void)
{
    ESP_LOGI(TAG, "OTA Manager Init");

    const esp_partition_t *running = esp_ota_get_running_partition();

    ESP_LOGI(TAG, "Running partition:");
    ESP_LOGI(TAG, "  - Label: %s", running->label);
    ESP_LOGI(TAG, "  - Address: 0x%lx", running->address);
    ESP_LOGI(TAG, "  - Size: %lu bytes", running->size);

    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(running, &app_desc) == ESP_OK) {
        ESP_LOGI(TAG, "  - Version: %s", app_desc.version);
        ESP_LOGI(TAG, "  - Compiled: %s %s", app_desc.date, app_desc.time);
    }

#ifdef OTA_SERVER_CERT_EMBEDDED
    ESP_LOGI(TAG, "HTTPS trust source: embedded OTA server certificate");
#elif defined(CONFIG_EXAMPLE_USE_CERT_BUNDLE)
    ESP_LOGI(TAG, "HTTPS trust source: ESP x509 certificate bundle");
#else
    ESP_LOGW(TAG, "HTTPS trust source is not configured");
#endif

    return ESP_OK;
}

esp_err_t ota_update(const char *url)
{
    ESP_LOGI(TAG, "Starting OTA Update");

    if (!ota_url_uses_https(url)) {
        ESP_LOGE(TAG, "OTA URL must use HTTPS with certificate validation: %s", url ? url : "(null)");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "URL: %s", url);

    esp_err_t err = ESP_OK;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[1/4] Finding next OTA partition...");

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "  Current: %s", running->label);
    ESP_LOGI(TAG, "  Target:  %s (0x%lx)", update_partition->label, update_partition->address);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[2/4] Connecting with HTTPS...");

#if !defined(OTA_SERVER_CERT_EMBEDDED) && !defined(CONFIG_EXAMPLE_USE_CERT_BUNDLE)
    ESP_LOGE(TAG, "No HTTPS trust source configured. Enable cert bundle or add certs/ota_server_cert.pem");
    return ESP_ERR_INVALID_STATE;
#endif

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
#ifdef OTA_SERVER_CERT_EMBEDDED
        .cert_pem = ota_server_cert_pem_start,
#elif defined(CONFIG_EXAMPLE_USE_CERT_BUNDLE)
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_begin failed: %s", esp_err_to_name(err));
        return err;
    }

    int status_code = esp_https_ota_get_status_code(https_ota_handle);
    if (status_code != 200) {
        ESP_LOGE(TAG, "Unexpected HTTP status code: %d", status_code);
        err = ESP_ERR_INVALID_RESPONSE;
        goto ota_abort;
    }

    int content_length = esp_https_ota_get_image_size(https_ota_handle);
    if (content_length <= 0) {
        ESP_LOGE(TAG, "Invalid firmware content length: %d", content_length);
        err = ESP_ERR_INVALID_SIZE;
        goto ota_abort;
    }

    if ((uint32_t)content_length > update_partition->size) {
        ESP_LOGE(TAG, "Firmware is larger than target partition: %d > %lu", content_length, update_partition->size);
        err = ESP_ERR_INVALID_SIZE;
        goto ota_abort;
    }

    ESP_LOGI(TAG, "  Firmware size: %d bytes", content_length);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[3/4] Verifying image header...");

    esp_app_desc_t new_app_info = {0};
    err = esp_https_ota_get_img_desc(https_ota_handle, &new_app_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed: %s", esp_err_to_name(err));
        goto ota_abort;
    }

    err = validate_image_header(&new_app_info);
    if (err == ESP_ERR_INVALID_VERSION) {
        esp_https_ota_abort(https_ota_handle);
        ESP_LOGI(TAG, "OTA skipped; device is already running this firmware version");
        return ESP_OK;
    } else if (err != ESP_OK) {
        goto ota_abort;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[4/4] Downloading and writing to %s...", update_partition->label);

    int last_logged = 0;
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);

        int binary_file_length = esp_https_ota_get_image_len_read(https_ota_handle);
        if (binary_file_length >= 0 && binary_file_length - last_logged >= OTA_PROGRESS_STEP_BYTES) {
            ESP_LOGI(TAG, "  Written: %d bytes", binary_file_length);
            last_logged = binary_file_length;
        }

        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_perform failed: %s", esp_err_to_name(err));
        goto ota_abort;
    }

    if (!esp_https_ota_is_complete_data_received(https_ota_handle)) {
        ESP_LOGE(TAG, "Complete OTA image was not received");
        err = ESP_ERR_INVALID_SIZE;
        goto ota_abort;
    }

    err = esp_https_ota_finish(https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_finish failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "OTA Update SUCCESS; rebooting into new partition");

    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();

    return ESP_OK;

ota_abort:
    if (https_ota_handle != NULL) {
        esp_https_ota_abort(https_ota_handle);
    }
    return err;
}
