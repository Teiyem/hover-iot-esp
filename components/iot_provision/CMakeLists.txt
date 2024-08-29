set(srcs "iot_provision.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "wifi_provisioning" "qrcode"
                    REQUIRES "esp_wifi" "iot_storage" "iot_common")