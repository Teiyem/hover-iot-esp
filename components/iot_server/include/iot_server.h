#pragma once

#include <functional>
#include <map>
#include "esp_http_server.h"
#include "iot_common.h"

/**
 * A class for handling networking with an HTTP server.
 */
class iot_server final
{
public:
    iot_server(void);
    ~iot_server(void);
    esp_err_t start(void);
    bool started() const { return _started; }
    esp_err_t register_route(const char *path, httpd_method_t method, esp_err_t(*handler)(httpd_req_t *r));
    esp_err_t valid_auth(httpd_req_t *req);
    esp_err_t send_res(httpd_req_t *req, const char *data, const char *status = "200");
    esp_err_t get_req(httpd_req_t *req, char *buf, size_t buf_len);
    esp_err_t get_query_string(httpd_req_t *req, char **query_string);
    esp_err_t send_err_response(httpd_req_t *req, httpd_err_code_t error, const char *err_message = DEFAULT_ERR_MSG);

private:
    static constexpr const char *TAG = "iot_server";                                     /**< A constant used to identify the source of the log message of this class. */
    static constexpr const char *API_KEY = "aesY}zeN]v4DOp@o2)-";                        /**< A temporay api key*/
    static constexpr const char *DEFAULT_ERR_MSG = "The request could not be processed"; /**< The default error message.*/
    static constexpr const char *MEDIA_TYPE_JSON = "application/json";                   /**< The media type string for JSON. */
    static constexpr const char *BASE_SERVER_PATH = "/api/v1/";                          /**< The base url path for the server. */
    const uint8_t MAX_QUERY_SIZE = 100;                                                  /**< The max query length size.*/
    bool _started = false;                                                               /**< Indicates whether the server has started or not.*/
    httpd_handle_t _server = nullptr;
};
