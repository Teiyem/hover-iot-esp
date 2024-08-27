idf_component_register(SRCS "iot_common.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_common" "esp_timer" "freertos" "esp_event" "json" "iot_component")
