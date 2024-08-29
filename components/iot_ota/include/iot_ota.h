#pragma once

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_http_client.h"
#include "iot_common.h"
#include "iot_server.h"
#include "iot_factory.h"
#include "iot_ota_defs.h"

/**
 * A class for handling ota related functionality.
 */
class IotOta final
{
public:
    IotOta(void);
    esp_err_t init(esp_app_desc_t *app_desc);

private:
    static constexpr const char *TAG = "IotOta";         /**< A constant used to identify the source of the log message of this class. */
    static iot_ota_state_e _ota_state;
    static esp_ota_handle_t _update_handle;
    static IotServer *_iot_server;
    static const esp_partition_t *_update_partition;
    static esp_app_desc_t *_app_info;
    static esp_err_t on_update(httpd_req_t *req);
    static esp_err_t on_status(httpd_req_t *req);
    static esp_err_t start(void);
    static esp_err_t write(char *buf, size_t buf_size, size_t *remaining);
    static esp_err_t end(void);
};