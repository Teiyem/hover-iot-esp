#include "iot_server.h"

/**
 * Initialises a new instance of the iot_server class.
 */
iot_server::iot_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    _server = nullptr;
    httpd_start(&_server, &config);
}

/**
 * Destructor for the iot_server.
 */
iot_server::~iot_server(void)
{
    if (_server != nullptr)
    {
        httpd_stop(_server);
    }
}

/**
 *  Register an HTTP route with the server.
 *
 * @param path The URL path to register.
 * @param handler The function to handle requests to the path.
 */
void iot_server::register_route(const char *path, httpd_method_t method, std::function<esp_err_t(httpd_req_t *)> handler)
{
    const httpd_uri_t uri_handler = {
        .uri = path,
        .method = method,
        .handler = [](httpd_req_t *req)
        {
            auto *self = static_cast<iot_server *>(req->user_ctx);
            return self->handle_route(req);
        },
        .user_ctx = this};

    httpd_register_uri_handler(_server, &uri_handler);

    _routes[path] = handler;
}

/**
 * Handle an HTTP request for a registered route.
 *
 * @param req The HTTP request to handle.
 * @return ESP_OK if the request was handled successfully, or an error code otherwise.
 */
esp_err_t iot_server::handle_route(httpd_req_t *req)
{
    const char *path(req->uri);
    if (_routes.find(path) == _routes.end())
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Route not found");
        return ESP_FAIL;
    }
    auto handler = _routes[path];
    return handler(req);
}