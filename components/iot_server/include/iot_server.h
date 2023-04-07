#pragma once

#include <functional>
#include <map>
#include <string>
#include "esp_http_server.h"

/**
 * A class for handling networking with an HTTP server.
 */
class iot_server
{
public:
    iot_server(void);
    ~iot_server(void);
    void register_route(const char *path, httpd_method_t method, std::function<esp_err_t(httpd_req_t *)> handler);

private:
    httpd_handle_t _server;                                                  /** The handle to the HTTP server instance. */
    std::map<const char *, std::function<esp_err_t(httpd_req_t *)>> _routes; /** The handle to the HTTP server instance. */
    esp_err_t handle_route(httpd_req_t *req);
};
