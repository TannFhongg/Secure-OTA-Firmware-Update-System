idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        app_update
        esp_wifi
        nvs_flash
        ota_manager
        protocol_examples_common
)
