#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "ota_manager.h"

static const char *TAG = "MAIN";

#define OTA_URL CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL

static void mark_running_app_valid_after_checkpoint(void)
{
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK &&
        ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "New OTA image is pending verification; validating after WiFi checkpoint");

        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA image marked valid; rollback cancelled");
        } else {
            ESP_LOGE(TAG, "Failed to mark OTA image valid: %s", esp_err_to_name(err));
        }
    }
#endif
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 OTA                            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Khởi tạo OTA và hiển thị partition info
    ESP_ERROR_CHECK(ota_init());
    
    // Khởi tạo network
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    
    ESP_LOGI(TAG, "WiFi connected!");

    mark_running_app_valid_after_checkpoint();

#if CONFIG_EXAMPLE_CONNECT_WIFI
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif

#if CONFIG_EXAMPLE_AUTO_OTA_ON_BOOT
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Auto OTA is enabled; checking for update at: %s", OTA_URL);

    esp_err_t err = ota_update(OTA_URL);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "Check HTTPS URL, server certificate trust, and firmware .bin availability");
    }
#else
    ESP_LOGI(TAG, "Auto OTA on boot is disabled; enable CONFIG_EXAMPLE_AUTO_OTA_ON_BOOT to update from %s", OTA_URL);
#endif
    
    // Nếu OTA thất bại, chạy app bình thường
    while (1) {
        ESP_LOGI(TAG, "App running... (heap: %lu)", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
