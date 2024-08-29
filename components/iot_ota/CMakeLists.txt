set(srcs "iot_ota.cpp")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "app_update" "esp_http_client"
                       PRIV_REQUIRES "iot_common" "iot_server")