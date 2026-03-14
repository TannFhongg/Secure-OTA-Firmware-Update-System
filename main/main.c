#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "ota_manager.h"

static const char *TAG = "MAIN";

// Cấu hình WiFi và OTA URL
#define WIFI_SSID      CONFIG_EXAMPLE_WIFI_SSID
#define WIFI_PASS      CONFIG_EXAMPLE_WIFI_PASSWORD
#define OTA_URL        CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL

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
    

    ESP_LOGI(TAG, "  1. Build firmware mới:");
    ESP_LOGI(TAG, "idf.py build");
   
    ESP_LOGI(TAG, "2. Chạy HTTP server:");
    ESP_LOGI(TAG, "python scripts/ota_server.py");
   
    ESP_LOGI(TAG, "3. Đợi 10 giây...");
   
    
    
    for (int i = 10; i > 0; i--) {
        ESP_LOGI(TAG, "OTA update sẽ bắt đầu sau %d giây...", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Bắt đầu OTA update!");
    
    // Thực hiện OTA
    esp_err_t err = ota_update(OTA_URL);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA update failed!");
        ESP_LOGE(TAG, "Kiểm tra:");
        ESP_LOGE(TAG, "  - HTTP server đang chạy?");
        ESP_LOGE(TAG, "  - URL đúng? %s", OTA_URL);
        ESP_LOGE(TAG, "  - File .bin tồn tại?");
    }
    
    // Nếu OTA thất bại, chạy app bình thường
    while (1) {
        ESP_LOGI(TAG, "App running... (heap: %lu)", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
