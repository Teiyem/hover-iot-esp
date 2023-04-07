idf_component_register(SRCS "iot_wifi.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "freertos" "esp_netif" "esp_event" "lwip" "esp_wifi" "esp_common")
