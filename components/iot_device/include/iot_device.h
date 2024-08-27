#pragma once

#include <typeinfo>
#include <cJSON.h>
#include "iot_server.h"
#include "iot_common.h"
#include "iot_factory.h"
#include "iot_device_defs.h"

class IotDevice final
{
public:
    IotDevice(void);
    ~IotDevice(void);
    esp_err_t init(iot_device_cfg_t *iot_device_cfg);

private:
    static constexpr const char *TAG = "IotDevice"; /* A constant used to identify the source of the log message of this. */
    static IotServer *_iot_server;
    static iot_device_cfg_t *_iot_device_cfg;

    esp_err_t validate_cfg(const iot_device_cfg_t *cfg);
    esp_err_t register_routes(httpd_method_t method);
    static esp_err_t _register_route(const char *path, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r));
    static esp_err_t on_write(httpd_req_t *req);
    static esp_err_t on_read(httpd_req_t *req);
    static esp_err_t on_info(httpd_req_t *req);
    static esp_err_t iot_ctl_write_from_proto(char *buf, size_t buf_len, iot_attribute_req_param_t *param);
    static std::string iot_device_type_to_str(iot_device_type_t type);
};