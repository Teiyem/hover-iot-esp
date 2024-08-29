set(srcs "iot_device.cpp" "iot_device_util.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "iot_mqtt"
                    REQUIRES "iot_common" "iot_server")
