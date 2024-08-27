set(srcs "iot_provision.cpp" "proto-c/iot_provision.pb-c.c")

set(priv_include_dirs proto-c)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                    PRIV_REQUIRES "iot_proto"  "wifi_provisioning" "qrcode"
                    REQUIRES "esp_wifi" "iot_storage" "iot_common")