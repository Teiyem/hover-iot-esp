idf_component_register(SRCS "iot_wifi.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_netif" "lwip" "esp_wifi" "iot_common" "iot_storage"
                    PRIV_REQUIRES  "mdns")
