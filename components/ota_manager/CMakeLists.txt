idf_component_register(
    SRCS "ota_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        app_update
        esp_app_format
        esp_http_client
        esp_https_ota
        mbedtls
)

set(OTA_SERVER_CERT "${PROJECT_DIR}/certs/ota_server_cert.pem")
if(EXISTS "${OTA_SERVER_CERT}")
    target_add_binary_data(${COMPONENT_LIB} "${OTA_SERVER_CERT}" TEXT)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE OTA_SERVER_CERT_EMBEDDED=1)
endif()
