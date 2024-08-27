#pragma once

#include <functional>
#include "esp_https_server.h"
#include "iot_common.h"
#include "iot_storage.h"
#include "iot_server_defs.h"

/**
 * A class for handling http server related functionalities.
 */
class IotServer final
{
public:
    IotServer(void);
    ~IotServer(void);
    IotServer(const IotServer&) = delete;
    IotServer(IotServer&&) = delete;
    IotServer& operator=(const IotServer&) = delete;
    IotServer& operator=(IotServer&&) = delete;

    esp_err_t start(void);
    void set_auth(std::string auth);
    [[nodiscard]] bool started() const { return _started; }
    esp_err_t register_route(const std::string route, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r));
    esp_err_t valid_auth(httpd_req_t *req);
    esp_err_t send_res(httpd_req_t *req, const char *data, size_t len, const char *status = "200");
    esp_err_t send_res(httpd_req_t *req, iot_response_t res);
    esp_err_t get_payload(httpd_req_t *req, char *buf, size_t buf_len);
    esp_err_t get_query_value(const char *query, const char *key, char **value);
    esp_err_t send_err(httpd_req_t *req, iot_error_response_t res);
    std::string get_path_param(httpd_req_t *p_req, std::string path);

private:
    static constexpr const char *TAG = "IotServer";                                      /**< A constant used to identify the source of the log message of this class. */
    static constexpr const char *API_KEY = "aesY}zeN]v4DOp@o2)-";                        /**< A temporary api key*/
    static constexpr const char *DEFAULT_ERR_MSG = "The request could not be processed"; /**< The default error message.*/
    static constexpr const char *BASE_SERVER_PATH = "/api/v1/device/";                   /**< The base url path for the server. */
    const uint8_t MAX_QUERY_VALUE_SIZE = 51;                                             /**< The max query length size.*/
    bool _started = false;                                                               /**< Indicates whether the server has started or not.*/
    std::string _api_key;
    httpd_handle_t _server;
    IotStorage *_iot_storage;
    std::string http_err_status(httpd_err_code_t error);
};
