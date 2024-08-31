idf_component_register(SRCS "iot_ota.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES "app_update"
                       PRIV_REQUIRES "iot_common" "iot_server"
)