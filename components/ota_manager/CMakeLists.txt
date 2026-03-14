idf_component_register(
    SRCS "ota_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_http_client
        app_update
        nvs_flash
)
