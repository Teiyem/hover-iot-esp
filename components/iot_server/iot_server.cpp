#include <cJSON.h>
#include "iot_server.h"
#include "iot_storage.h"

std::string IotServer::_api_key{};

/**
 * Initialises a new instance of the IotServer class.
 */
IotServer::IotServer(void)
{
    _server = nullptr;
}

/**
 * Destroys the IotServer class.
 */
IotServer::~IotServer(void)
{
    stop();
}

/**
 * Starts the http server.
 *
 * @return ESP_OK on success, otherwise an error code
 */

esp_err_t IotServer::start(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "%s: Starting component", __func__);

    if (_started) {
        ESP_LOGW(TAG, "%s: Component is already started", __func__);
        return ESP_OK;
    }

#if CONFIG_IOT_HOVER_SERVER_HTTPS
    auto storage = IotFactory::create_scoped<IotStorage>(IOT_NVS_FACTORY_PART_NAME,
                                                         IOT_NVS_FACTORY_NAMESPACE);

    size_t cert_len = 0;
    uint8_t *cert = nullptr;

    ret = storage->read("cert", reinterpret_cast<void **>(&cert), cert_len, TYPE_STR);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Failed to get cert [reason: %s] ", __func__, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "%s: Got cert [length: %d, data: %s]", __func__, cert_len, (char *)cert);

    size_t pvt_key_len = 0;
    uint8_t *pvt_key = nullptr;

    ret = storage->read("pvt_key", reinterpret_cast<void **>(&pvt_key), pvt_key_len, TYPE_STR);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Failed to get private key [reason: %s] ", __func__, esp_err_to_name(ret));
        return ret;
    }

    size_t ca_cert_len = 0;
    uint8_t *ca_cert = nullptr;

    ret = storage->read("ca_cert", reinterpret_cast<void **>(&ca_cert), ca_cert_len, TYPE_STR);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Failed to get private key [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

    config.servercert = cert;
    config.servercert_len = cert_len;
    config.prvtkey_pem = pvt_key;
    config.prvtkey_len = pvt_key_len;
    config.cacert_pem = ca_cert;
    config.cacert_len = ca_cert_len;
    config.httpd.uri_match_fn = httpd_uri_match_wildcard;

    ret = httpd_ssl_start(&_server, &config);
#elif CONFIG_IOT_HOVER_SERVER_HTTP
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 4;
    config.stack_size = 8192;
    config.max_uri_handlers = 10;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ret = httpd_start(&_server, &config);
#endif

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to start http server [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    _started = true;

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);

    return ret;
}

/**
 * Stops the components.
 */
void IotServer::stop(void)
{
    if (_server != nullptr)
        httpd_stop(_server);
}

/**
 * Sets the api key to use to validate request.
 *
 * @param[in] auth The api key to set. If empty default will be used.
 */
void IotServer::set_auth(std::string auth)
{
    if (auth.empty())
        _api_key = API_KEY;
    else
        _api_key = auth;

    ESP_LOGI(TAG, "%s: API KEY: %s", __func__, _api_key.c_str());
}

/**
 * Register an http route with the server.
 *
 * @param[in] path   The URL path to register.
 * @param[in] method The http method of the route to register.
 * @param[in] handler The function to handle requests to the path.
 * @returns ESP_OK on success, ESP_FAIL or ESP_ERR_INVALID_ARG on failure.
 */
esp_err_t IotServer::register_route(const std::string path, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r))
{
    ESP_LOGI(TAG, "%s: Registering route", __func__);

    if (handler == nullptr) {
        ESP_LOGE(TAG, "%s: Handler cannot be null", __func__);
        return ESP_ERR_INVALID_ARG;
    }

    if (!iot_valid_str(path.c_str()))
        return ESP_ERR_INVALID_ARG;

    std::string uri = BASE_SERVER_PATH + path;

    const httpd_uri_t uri_handler = {
            .uri = uri.c_str(),
            .method = method,
            .handler = on_auth,
            .user_ctx = (void *)handler
    };

    esp_err_t ret = httpd_register_uri_handler(_server, &uri_handler);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to register route [path: %s, reason: %s]", __func__, uri.c_str(),
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "%s: Successfully registered route [path: %s]", __func__, uri.c_str());

    return ret;
}

/**
 * Checks the request header for valid authorization.
 *
 * @param[in] req A pointer to the http request object.
 * @return ESP_OK on success, ESP_FAIL request is not authorized.
 */
esp_err_t IotServer::on_auth(httpd_req_t *req)
{
    ESP_LOGI(TAG, "%s: Verifying if the request is authorized", __func__);

    const char *hdr_key = "X-API-KEY";

    esp_err_t ret = ESP_FAIL;

    size_t hdr_len = httpd_req_get_hdr_value_len(req, hdr_key);

    if (hdr_len < 1) {
        ESP_LOGE(TAG, "%s: Couldn't find header [name: %s ]", __func__, hdr_key);
        httpd_resp_send_err(req,  HTTPD_401_UNAUTHORIZED, nullptr);
        return ret;
    }

    hdr_len += 1;

    ESP_LOGI(TAG, "%s: Found header [name: %s]", __func__, hdr_key);

    char *api_key = iot_allocate_mem<char>(hdr_len);

    if (api_key == nullptr)
        return ret;

    ret = httpd_req_get_hdr_value_str(req, hdr_key, api_key, hdr_len);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Couldn't GET header [name: %s ] value", __func__, hdr_key);
        iot_free_one(api_key);
        httpd_resp_send_err(req,  HTTPD_401_UNAUTHORIZED, nullptr);
        return ret;
    }

    ESP_LOGI(TAG, "%s: %s Header value retrieved, verifying match", __func__, hdr_key);

    ret = _api_key == api_key ? ESP_OK : ESP_FAIL;

    ESP_LOGI(TAG, "%s: Request [state: %s]", __func__, ret == ESP_OK ? "authorized" : "unauthorized");

    iot_free_one(api_key);

    if (ret == ESP_OK) {
        iot_not_null(req->user_ctx);
        auto handler = reinterpret_cast<esp_err_t (*)(httpd_req_t *)>(req->user_ctx);
        return handler(req);
    }

    httpd_resp_send_err(req,  HTTPD_401_UNAUTHORIZED, nullptr);

    return ret;
}

/**
 * Sends an http success response.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] body The response data.
 * @param[in] status The response status. Default is 200.
 * @param[in] message Whether the response body is just plain text and not json. Default is false
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t IotServer::send_res(httpd_req_t *req, const char *data, bool message, iot_http_status_e status)
{
    httpd_resp_set_status(req, std::to_string(status).c_str());

    bool parse_error = false;

    char *buf;

    cJSON *res = cJSON_CreateObject();

    if (res == nullptr) {
        parse_error = true;
    } else {
        if (message)
            cJSON_AddStringToObject(res, "message", data);
        else {
            if (data != nullptr)
                cJSON_AddRawToObject(res, "data", data);
        }

        cJSON_AddNumberToObject(res, "status", status);
        cJSON_AddStringToObject(res, "timestamp", iot_now_str().data());
    }

    if (!parse_error) {
        buf = cJSON_Print(res);

        if (buf == nullptr)
            parse_error = true;
    }

    esp_err_t ret;

    size_t len =  data == nullptr ? 0 : HTTPD_RESP_USE_STRLEN;

    if (parse_error) {
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        ret = httpd_resp_send(req,  data, len);
    } else {
        httpd_resp_set_type(req, HTTPD_TYPE_TEXT);

        ret = httpd_resp_send(req, buf, len);

        iot_free(buf);
        cJSON_Delete(res);
    }

    return ret;
}

/**
 * Gets the request body.
 *
 * @param[in] req A pointer to the http request object.
 * @param[out] buf A pointer to the buffer to store the request body.
 * @param[in] buf_len The length of the buffer.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotServer::get_body(httpd_req_t *req, char *buf, size_t buf_len)
{
    ESP_LOGI(TAG, "%s: Reading payload", __func__);

    int ret = httpd_req_recv(req, buf, buf_len);

    if (ret <= 0) {
        ESP_LOGE(TAG, "%s: Failed to read payload [reason: %s]", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    buf[buf_len] = '\0';

    ESP_LOGI(TAG, "%s: Successfully read [payload: %s]", __func__, buf);

    return ESP_OK;
}

/**
 * Sends an error response.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] message The error message. Default is DEFAULT_ERR_MSG define.
 * @param[in] status The error status. Default is 500
 * @return ESP_FAIL.
 */
esp_err_t IotServer::send_err(httpd_req_t *req, const char *message, iot_http_status_e status)
{
    bool parse_error = false;

    char *buf;

    cJSON *res = cJSON_CreateObject();

    if (res == nullptr) {
        parse_error = true;
    } else {
        cJSON_AddStringToObject(res, "problem", message);
        cJSON_AddNumberToObject(res, "status", status);
        cJSON_AddStringToObject(res, "timestamp", iot_now_str().data());
    }
    
    if (!parse_error) {
        buf = cJSON_Print(res);

        if (buf == nullptr)
            parse_error = true;
    }

    if (parse_error) {
        httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
        httpd_resp_send_err(req,  HTTPD_500_INTERNAL_SERVER_ERROR, message);
    } else {
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_set_status(req, std::to_string(status).c_str());

        httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);

        cJSON_free(buf);
        cJSON_Delete(res);
    }

    return ESP_FAIL;
}

/**
 * Get the URL query value string from an URL query string.
 *
 * @param[in] query  A pointer to the URL query string.
 * @param[in] key    A pointer to the URL query string key.
 * @param[out] value A pointer to the buf to store the URL query string.
 * @return ESP_OK on success, otherwise an error code
 * @note Ensure to free the query_string after usage.
 */
esp_err_t IotServer::get_query_value(const char *query, const char *key, char **value)
{
    char *buf = iot_allocate_mem<char>(MAX_QUERY_VALUE_SIZE);

    esp_err_t ret = httpd_query_key_value(query, key, buf, MAX_QUERY_VALUE_SIZE);

    if (ret != ESP_OK) {
        free(buf);
        ESP_LOGE(TAG, "%s: Error getting value from -> %s", __func__, query);
        return ret;
    }

    buf[MAX_QUERY_VALUE_SIZE] = '\0';

    *value = buf;
    return ESP_OK;
}

/**
 * Gets the path parameter or resource identifier from the requested url.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] path The path to search for within the url.
 * @return std::string The path parameter if found, otherwise an empty string.
 */
std::string IotServer::get_path_param(httpd_req_t *req, std::string path)
{
    ESP_LOGI(TAG, "%s: Getting path param from [url:  %s]", __func__, req->uri);

    std::string uri(req->uri);

    size_t start = uri.find(path);

    if (start != std::string::npos) {
        return uri.substr(start + path.length());
    }

    ESP_LOGW(TAG, "%s: Path param not found for the [path: %s]", __func__, path.c_str());

    return "";
}