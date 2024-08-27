set(srcs "iot_device.cpp" "iot_device_util.cpp" "proto-c/iot_device.pb-c.c")

set(priv_include_dirs priv proto-c)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                    PRIV_REQUIRES protobuf-c
                    REQUIRES "iot_common" "iot_server")
