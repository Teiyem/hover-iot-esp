#include "iot_server.h"

/**
 * Initialises a new instance of the iot_server class.
 */
iot_server::iot_server(void)
{
    _server = nullptr;
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
 * Starts the http server.
 * @returns ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t iot_server::start(void)
{
    esp_err_t ret = ESP_FAIL;

    ESP_LOGI(tag, "%s -> Starting the device http server", __func__);

    if (_server != nullptr)
    {
        ESP_LOGI(tag, "%s -> Device http server is already started", __func__);
        return ret;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&_server, &config) != ESP_OK)
    {
        ESP_LOGI(tag, "%s -> Failed to start http server", __func__);
        return ret;
    }

    return ESP_OK;
}

/**
 * Checks the request header for authorization headers and a valid api-key.
 * @param req The request object.
 * @return An esp_err_t result of the.
 * @return ESP_OK on success, ESP_FAIL request is not authorized.
 */
esp_err_t iot_server::validate_auth(httpd_req_t *req)
{
    ESP_LOGI(tag, "%s -> Verifying if the request is authorized", __func__);

    const char *header = "x-api-key";

    esp_err_t ret = ESP_FAIL;

    auto header_len = httpd_req_get_hdr_value_len(req, header);

    if (header_len < 1)
    {
        ESP_LOGE(tag, "%s -> Couldn't find the -> %s header", __func__, header);
        return ret;
    }

    header_len += 1;

    ESP_LOGI(tag, "%s -> Found -> %s header", __func__, header);

    auto *api_key = static_cast<char *>(allocate_mem(header_len));

    if (api_key == nullptr)
    {
        return ret;
    }

    ESP_LOGI(tag, "%s -> %s Header value retrieved, verifying match", __func__, header);

    ret = strcmp(_api_key, api_key) == 0 ? ESP_OK : ESP_FAIL;

    ESP_LOGI(tag, "%s -> Request is -> %s", __func__, ret == ESP_OK ? "authorized" : "unauthorized");

    free(api_key);

    return ret;
}

/**
 *  Register an HTTP route with the server.
 *
 * @param path The URL path to register.
 * @param handler The function to handle requests to the path.
 * @returns ESP_OK on success, ESP_FAIL or ESP_ERR_INVALID_ARG on failure.
 */
esp_err_t iot_server::register_route(const char *path, httpd_method_t method, std::function<esp_err_t(httpd_req_t *)> handler)
{
    ESP_LOGI(tag, "%s -> Registering route", __func__);

    if (check_string_validity(path) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const httpd_uri_t uri_handler = {
        .uri = path,
        .method = method,
        .handler = [](httpd_req_t *req)
        {
            auto *self = static_cast<iot_server *>(req->user_ctx);
            return self->handle_route(req);
        },
        .user_ctx = this};

    esp_err_t ret = httpd_register_uri_handler(_server, &uri_handler);

    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "%s -> Failed to register route -> %s", __func__, path);
        return ret;
    }

    _routes[path] = handler;

    ESP_LOGI(tag, "%s -> Successfully registered route -> %s", __func__, path);

    return ret;
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