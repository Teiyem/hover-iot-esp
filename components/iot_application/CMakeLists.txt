idf_component_register(SRCS "iot_application.cpp"
        INCLUDE_DIRS "include"
        REQUIRES "iot_common" "iot_wifi" "iot_device" "esp_app_format"
        "iot_server" "iot_mqtt"  "iot_provision" "iot_ota" "iot_status_led")