#pragma once

#include <functional>
#include "esp_https_server.h"
#include "iot_common.h"
#include "iot_component.h"
#include "iot_factory.h"
#include "iot_server_defs.h"

/**
 * A class for handling http server related functionalities.
 */
class IotServer final : public IotComponent
{
public:
    IotServer(void);
    ~IotServer(void);

    IotServer(const IotServer&) = delete;
    IotServer(IotServer&&) = delete;
    IotServer& operator=(const IotServer&) = delete;
    IotServer& operator=(IotServer&&) = delete;

    esp_err_t start(void) override;
    void stop(void) override;
    void set_auth(std::string auth);
    esp_err_t register_route(const std::string route, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r));
    static esp_err_t on_auth(httpd_req_t *req);
    esp_err_t send_res(httpd_req_t *req, const char *body, bool message = false, iot_http_status_e status = IOT_HTTP_STATUS_200_OK);
    esp_err_t get_body(httpd_req_t *req, char *buf, size_t buf_len);
    esp_err_t get_query_value(const char *query, const char *key, char **value);
    esp_err_t send_err(httpd_req_t *req, const char *error, iot_http_status_e status = IOT_HTTP_STATUS_500_INT_SERVER_ERROR);
    std::string get_path_param(httpd_req_t *p_req, std::string path);

private:
    static constexpr const char *TAG = "IotServer";                                      /**< A constant used to identify the source of the log message of this class. */
    static constexpr const char *API_KEY = "aesY}zeN]v4DOp@o2)-";                        /**< A temporary api key*/
    static constexpr const char *DEFAULT_ERR_MSG = "The request could not be processed"; /**< The default error message.*/
    static constexpr const char *BASE_SERVER_PATH = "/api/v1/device/";                   /**< The base url path for the server. */
    const uint8_t MAX_QUERY_VALUE_SIZE = 51;                                             /**< The max query length size.*/
    static std::string _api_key;
    httpd_handle_t _server;
};
