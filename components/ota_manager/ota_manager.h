
#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ota_init(void);

/**
Thực hiện OTA update đơn giản
 * 
Quy trình:
1. Kết nối HTTP server
2. Tải firmware
3. Ghi vào partition trống (ota_0 hoặc ota_1)
4. Đánh dấu partition mới là boot partition
5. Reboot
 
URL của firmware (HTTP)
return ESP_OK nếu thành công
 */
esp_err_t ota_update(const char *url);

#ifdef __cplusplus
}
#endif

#endif
