set(srcs "iot_ota.cpp" "proto-c/iot_ota.pb-c.c")

set(priv_include_dirs proto-c)

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                       REQUIRES "app_update" "esp_http_client"
                       PRIV_REQUIRES "iot_common" "iot_server" "iot_proto" "iot_appplication")