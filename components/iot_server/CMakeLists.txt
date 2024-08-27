idf_component_register(SRCS "iot_server.cpp" "iot_server_util.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_https_server" "iot_storage"
                    PRIV_REQUIRES "iot_common" "iot_proto")
