idf_component_register(SRCS "iot_mqtt.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES "iot_common" "mqtt"
                       EMBED_TXTFILES conf/iot_mqtt)