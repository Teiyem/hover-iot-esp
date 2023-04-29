#pragma once

#include <functional>
#include <map>
#include <string>
#include "esp_http_server.h"
#include "iot_util.h"

/**
 * A class for handling networking with an HTTP server.
 */
class iot_server
{
public:
    iot_server(void);
    ~iot_server(void);
    esp_err_t start(void);
    esp_err_t validate_auth(httpd_req_t *req);
    esp_err_t register_route(const char *path, httpd_method_t method, std::function<esp_err_t(httpd_req_t *)> handler);

private:
    static constexpr const char *tag = "iot_server";                         /** A constant used to identify the source of the log message of this class. */
    static constexpr const char *_api_key = "aesY}zeN]v4DOp@o2)-";           /** Temporay api key*/
    httpd_handle_t _server;                                                  /** The handle to the HTTP server instance. */
    std::map<const char *, std::function<esp_err_t(httpd_req_t *)>> _routes; /** The handle to the HTTP server instance. */
    esp_err_t handle_route(httpd_req_t *req);
};
