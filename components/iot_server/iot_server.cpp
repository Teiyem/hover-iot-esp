#include "iot_server.h"

/**
 * Initialises a new instance of the iot_server class.
 */
iot_server::iot_server(void)
{
    _server = nullptr;
}

/**
 * Destroys the iot_server class.
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
 *
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t iot_server::start(void)
{
    esp_err_t ret = ESP_FAIL;

    ESP_LOGI(TAG, "%s -> Starting component", __func__);

    if (_started)
    {
        ESP_LOGW(TAG, "%s -> Component is already started", __func__);
        return ESP_FAIL;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 4;
    config.stack_size = 8192;
    config.max_uri_handlers = 10;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    ret = httpd_start(&_server, &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s -> Failed to start http server reason -> %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    _started = true;

    ESP_LOGI(TAG, "%s -> Finished starting component", __func__);

    return ret;
}

/**
 * Register an http route with the server.
 *
 * @param[in] path The URL path to register.
 * @param[in] handler The function to handle requests to the path.
 * @returns ESP_OK on success, ESP_FAIL or ESP_ERR_INVALID_ARG on failure.
 */
esp_err_t iot_server::register_route(const char *path, httpd_method_t method, esp_err_t(*handler)(httpd_req_t *r))
{
    ESP_LOGI(TAG, "%s -> Registering route", __func__);

    if (handler == nullptr)
    {
        ESP_LOGE(TAG, "%s -> Handler cannot be null", __func__);
        return ESP_ERR_INVALID_ARG;
    }

    if (iot_verify_string(path) != ESP_OK)
        return ESP_ERR_INVALID_ARG;

    char uri[strlen(BASE_SERVER_PATH) + strlen(path) + 1];
    strcpy(uri, BASE_SERVER_PATH);
    strcat(uri, path);

    const httpd_uri_t uri_handler = {
        .uri = uri,
        .method = method,
        .handler = handler,
        .user_ctx = nullptr
    };

    esp_err_t ret = httpd_register_uri_handler(_server, &uri_handler);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s -> Failed to register route -> %s, reason -> %s", __func__, uri, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "%s -> Successfully registered route -> %s", __func__, uri);

    return ret;
}

/**
 * Checks the request header for authorization headers and a valid api-key.
 *
 * @param[in] req A pointer to the http request object.
 * @return ESP_OK on success, ESP_FAIL request is not authorized.
 */
esp_err_t iot_server::valid_auth(httpd_req_t *req)
{
    ESP_LOGI(TAG, "%s -> Verifying if the request is authorized", __func__);

    const char *header = "x-api-key";

    esp_err_t ret = ESP_FAIL;

    size_t header_len = httpd_req_get_hdr_value_len(req, header);

    if (header_len < 1)
    {
        ESP_LOGE(TAG, "%s -> Couldn't find the -> %s header", __func__, header);
        return ret;
    }

    header_len += 1;

    ESP_LOGI(TAG, "%s -> Found -> %s header", __func__, header);

    char *api_key = static_cast<char *>(iot_allocate_mem(header_len));

    if (api_key == nullptr)
        return ret;

    ESP_LOGI(TAG, "%s -> %s Header value retrieved, verifying match", __func__, header);

    ret = strcmp(API_KEY, api_key) == 0 ? ESP_OK : ESP_FAIL;

    ESP_LOGI(TAG, "%s -> Request is -> %s", __func__, ret == ESP_OK ? "authorized" : "unauthorized");

    free(api_key);

    return ret;
}

/**
 * Sends a response with the given data and HTTP status code over the provided request.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] dataThe The string data to include in the response body.
 * @param[in] status The HTTP status code to set in the response, Default "200" OK.
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t iot_server::send_res(httpd_req_t *req, const char *data, const char *status)
{
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, MEDIA_TYPE_JSON);

    if (data != nullptr)
        return httpd_resp_send(req, data, strlen(data));

    return httpd_resp_send(req, nullptr, 0);
}

/**
 * Gets an http request and reads its payload data.
 *
 * @param[in] req A pointer to the http request object.
 * @param[out] buf A pointer to the buffer where the payload data will be stored.
 * @param[in] buf_len The length of the buffer.
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t iot_server::get_req(httpd_req_t *req, char *buf, size_t buf_len)
{
    ESP_LOGI(TAG, "%s -> Reading payload", __func__);

    int ret = httpd_req_recv(req, buf, buf_len);

    if (ret <= 0)
    {
        ESP_LOGE(TAG, "%s -> Failed to read payload reason -> %s", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    buf[buf_len] = '\0';

    ESP_LOGI(TAG, "%s -> Successfully read payload -> %s", __func__, buf);

    return ESP_OK;
}

/**
 * Get the URL query string from an http request.
 *
 * @param[in] req A pointer to the http request object.
 * @param[out] query_string A pointer to the buf to store the URL query string.
 * @return ESP_OK on success, otherwise an error code
 * @note Ensure to free the query_string after usage.
 */
esp_err_t iot_server::get_query_string(httpd_req_t *req, char **query_string)
{
    size_t len = httpd_req_get_url_query_len(req);

    if (len == 0)
    {
        ESP_LOGE(TAG, "%s -> Size of url query is zero", __func__);
        return ESP_ERR_INVALID_SIZE;
    }

    len += 1;

    char *buf = static_cast<char *>(iot_allocate_mem(len));

    esp_err_t ret = httpd_req_get_url_query_str(req, buf, len);

    buf[len] = '\0';

    if (ret != ESP_OK)
    {
        free(buf);
        ESP_LOGE(TAG, "%s -> Error getting url query string", __func__);
        return ret;
    }

    *query_string = buf;
    return ESP_OK;
}

/**
 * Sends an http error response with the given error message and http status code.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] error The http error code to set in the response.
 * @param[in] err_message An optional error message to include in the response body.
 * @return ESP_OK.
 */
esp_err_t iot_server::send_err_response(httpd_req_t *req, httpd_err_code_t error, const char *err_message)
{
    httpd_resp_send_err(req, error, err_message);
    return ESP_OK;
}