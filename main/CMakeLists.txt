idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "esp_common" "iot_storage" "iot_server")