idf_component_register(SRCS "iot_server.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_https_server"
                    PRIV_REQUIRES "iot_common" "iot_storage")
