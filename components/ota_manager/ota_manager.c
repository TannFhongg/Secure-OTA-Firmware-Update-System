
#include "ota_manager.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_app_format.h"

static const char *TAG = "OTA_SIMPLE";

#define BUFFER_SIZE 1024

esp_err_t ota_init(void)
{
    ESP_LOGI(TAG, "OTA Manager Init");
    
    // Lấy thông tin partition đang chạy
    const esp_partition_t *running = esp_ota_get_running_partition();
    
    ESP_LOGI(TAG, "Running partition:");
    ESP_LOGI(TAG, "  - Label: %s", running->label);
    ESP_LOGI(TAG, "  - Address: 0x%lx", running->address);
    ESP_LOGI(TAG, "  - Size: %lu bytes", running->size);
    
    // Lấy thông tin app
    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(running, &app_desc) == ESP_OK) {
        ESP_LOGI(TAG, "  - Version: %s", app_desc.version);
        ESP_LOGI(TAG, "  - Compiled: %s %s", app_desc.date, app_desc.time);
    }
    
    return ESP_OK;
}

esp_err_t ota_update(const char *url)
{

    ESP_LOGI(TAG, "Starting OTA Update");
   
    ESP_LOGI(TAG, "URL: %s", url);
    
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    
    // BƯỚC 1: Tìm partition để ghi firmware mới
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[1/4] Finding next OTA partition...");
    
    const esp_partition_t *running = esp_ota_get_running_partition();
    update_partition = esp_ota_get_next_update_partition(NULL);
    
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found!");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "  Current: %s", running->label);
    ESP_LOGI(TAG, "  Target:  %s (0x%lx)", update_partition->label, update_partition->address);
    
    // BƯỚC 2: Kết nối HTTP và tải firmware
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[2/4] Downloading firmware...");
    
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 30000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }
    
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    
    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "  Firmware size: %d bytes", content_length);
    
    // BƯỚC 3: Bắt đầu ghi vào partition
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[3/4] Writing to partition %s...", update_partition->label);
    
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    
    char *buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Cannot allocate buffer");
        esp_ota_abort(update_handle);
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }
    
    int binary_file_length = 0;
    
    while (1) {
        int data_read = esp_http_client_read(client, buffer, BUFFER_SIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: data read error");
            break;
        } else if (data_read > 0) {
            err = esp_ota_write(update_handle, (const void *)buffer, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
                break;
            }
            binary_file_length += data_read;
            
            // In progress mỗi 50KB
            if (binary_file_length % (50 * 1024) == 0) {
                ESP_LOGI(TAG, "  Written: %d bytes", binary_file_length);
            }
        } else if (data_read == 0) {
            ESP_LOGI(TAG, "  Download complete: %d bytes", binary_file_length);
            break;
        }
    }
    
    free(buffer);
    esp_http_client_cleanup(client);
    
    if (err != ESP_OK) {
        esp_ota_abort(update_handle);
        return err;
    }
    
    // Kết thúc OTA
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // BƯỚC 4: Đặt partition mới làm boot partition
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "[4/4] Setting boot partition...");
    
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "  Boot partition set to: %s", update_partition->label);
    
    ESP_LOGI(TAG, "  OTA Update SUCCESS!");
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
    
    return ESP_OK;
}
