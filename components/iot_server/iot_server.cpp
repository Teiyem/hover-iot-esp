#include "iot_server.h"
#include "iot_common.pb-c.h"

/**
 * Initialises a new instance of the IotServer class.
 */
IotServer::IotServer(void)
{
    _server = nullptr;
    _iot_storage = new IotStorage(IOT_NVS_FACTORY_PART_NAME, IOT_NVS_FACTORY_NAMESPACE);
}

/**
 * Destroys the IotServer class.
 */
IotServer::~IotServer(void)
{
    if (_server != nullptr)
        httpd_stop(_server);
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

//    size_t cert_len = 0;
//    uint8_t *cert = nullptr;
//
//    if(_iot_storage == nullptr)
//    {
//        _iot_storage = new IotStorage("factory_nvs", "IotFactory");
//    }
//
//    ret = _iot_storage->read("cert", reinterpret_cast<void **>(&cert), cert_len);
//
//    if(ret != ESP_OK)
//    {
//        ESP_LOGE(TAG, "%s: Failed to get cert -> %s ", __func__, esp_err_to_name(ret));
//        return ret;
//    }
//
//    ESP_LOGI(TAG, "%s: Got cert with length of -> %d is -> %s", __func__, cert_len, (char *)cert);
//
//    size_t pvt_key_len = 0;
//    uint8_t *pvt_key = nullptr;
//
//    ret = _iot_storage->read("pvt_key", reinterpret_cast<void **>(&pvt_key), pvt_key_len);
//
//    if(ret != ESP_OK)
//    {
//        ESP_LOGE(TAG, "%s: Failed to get private key -> %s", __func__, esp_err_to_name(ret));
//        return ret;
//    }
//
//    size_t ca_cert_len = 0;
//    uint8_t *ca_cert = nullptr;
//
//    ret = _iot_storage->read("ca_cert", reinterpret_cast<void **>(&ca_cert), ca_cert_len);
//
//    if(ret != ESP_OK)
//    {
//        ESP_LOGE(TAG, "%s: Failed to get private key -> %s", __func__, esp_err_to_name(ret));
//        return ret;
//    }

//    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
//
//    config.servercert = cert;
//    config.servercert_len = cert_len;
//    config.prvtkey_pem = pvt_key;
//    config.prvtkey_len = pvt_key_len;
//    config.cacert_pem = ca_cert;
//    config.cacert_len = ca_cert_len;
//
//    ret = httpd_ssl_start(&_server, &config);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 4;
    config.stack_size = 8192;
    config.max_uri_handlers = 10;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    config.uri_match_fn = httpd_uri_match_wildcard;

    ret = httpd_start(&_server, &config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to start http server reason -> %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    _started = true;

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);

    return ret;
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
            .handler = handler,
            .user_ctx = nullptr
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
 * Checks the request header for authorization headers and a valid api-key.
 *
 * @param[in] req A pointer to the http request object.
 * @return ESP_OK on success, ESP_FAIL request is not authorized.
 */
esp_err_t IotServer::valid_auth(httpd_req_t *req)
{
    ESP_LOGI(TAG, "%s: Verifying if the request is authorized", __func__);

    const char *header = "X-API-KEY";

    esp_err_t ret = ESP_FAIL;

    size_t header_len = httpd_req_get_hdr_value_len(req, header);

    if (header_len < 1) {
        ESP_LOGE(TAG, "%s: Couldn't find the -> %s header", __func__, header);
        return ret;
    }

    header_len += 1;

    ESP_LOGI(TAG, "%s: Found -> %s header", __func__, header);

    char *api_key = iot_allocate_mem<char>(header_len);

    if (api_key == nullptr)
        return ret;

    ESP_LOGI(TAG, "%s: %s Header value retrieved, verifying match", __func__, header);

    ret = _api_key == api_key ? ESP_OK : ESP_FAIL;

    ESP_LOGI(TAG, "%s: Request is -> %s", __func__, ret == ESP_OK ? "authorized" : "unauthorized");

    free(api_key);

    return ret;
}

/**
 * Sends an http success response to the request.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] dataThe The string data to include in the response body.
 * @param[in] status The HTTP status code to set in the response, Default "200" OK.
 * @param[in] len    The length of the response data.
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t IotServer::send_res(httpd_req_t *req, const char *data, size_t len, const char *status)
{
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, IOT_HTTPD_TYPE_JSON);

    if (data != nullptr) {
        if (len == 0) {
            len = strlen(data);
        }

        return httpd_resp_send(req, data, len);
    }

    return httpd_resp_send(req, nullptr, 0);
}

/**
 * Sends an http success response to the request.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] res The response data.
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t IotServer::send_res(httpd_req_t *req, iot_response_t res)
{
    httpd_resp_set_status(req, res.status);

    switch (res.type) {
        case IOT_RESPONSE_TYPE_JSON:
            httpd_resp_set_type(req, IOT_HTTPD_TYPE_JSON);
            break;
        case IOT_RESPONSE_TYPE_DEFAULT:
            httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
            break;
        case IOT_RESPONSE_TYPE_PROTO:
            httpd_resp_set_type(req, IOT_HTTPD_TYPE_PROTO);
            break;
    }

    if (res.data != nullptr) {
        size_t len = res.len;

        if (len == 0)
            len = strlen(res.data);

        esp_err_t ret = httpd_resp_send(req, res.data, len);

        if (res.free_data) {
            free(res.data);
        }

        return ret;
    }

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
esp_err_t IotServer::get_payload(httpd_req_t *req, char *buf, size_t buf_len)
{
    ESP_LOGI(TAG, "%s: Reading payload", __func__);

    int ret = httpd_req_recv(req, buf, buf_len);

    if (ret <= 0) {
        ESP_LOGE(TAG, "%s: Failed to read payload [reason: %s]", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

//    buf[buf_len] = '\0';

    ESP_LOGI(TAG, "%s: Successfully read [payload: %s]", __func__, buf);

    return ESP_OK;
}

/**
 * Sends an error response.
 *
 * @param[in] req A pointer to the http request object.
 * @param[in] res The error response data.
 * @return ESP_OK.
 */
esp_err_t IotServer::send_err(httpd_req_t *req, iot_error_response_t res)
{
    char *buf = nullptr;
    size_t len = HTTPD_RESP_USE_STRLEN;

    if (res.type == IOT_RESPONSE_TYPE_PROTO) {
        IotResponse response = IOT_RESPONSE__INIT;

        if (res.error == HTTPD_400_BAD_REQUEST)
            response.status = IOT_RESPONSE_STATUS__REJECTED;

        if (res.error == HTTPD_500_INTERNAL_SERVER_ERROR)
            response.status = IOT_RESPONSE_STATUS__FAILED;

        if (res.message != nullptr)
            response.message = res.message;

        len = iot_response__get_packed_size(&response);

        buf = iot_allocate_mem<char>(len);

        if (buf != nullptr)
            iot_response__pack(&response, reinterpret_cast<uint8_t *>(buf));
        else
            res.type = IOT_RESPONSE_TYPE_DEFAULT; // Send text
    }

    if (res.type == IOT_RESPONSE_TYPE_DEFAULT) {
        httpd_resp_send_err(req, res.error, res.message);
        return ESP_OK;
    }

    if (res.type == IOT_RESPONSE_TYPE_PROTO)
        httpd_resp_set_type(req, IOT_HTTPD_TYPE_PROTO);

    if (res.type == IOT_RESPONSE_TYPE_JSON)
        httpd_resp_set_type(req, IOT_HTTPD_TYPE_PROTO);

    send_res(req, buf, len, http_err_status(res.error).c_str());

    if (res.free_msg) {
        if (res.type == IOT_RESPONSE_TYPE_JSON)
            free(res.message);
        else {
            if (buf != nullptr)
                free(buf);
        }

    }

    return ESP_OK;
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


/**
 * Gets the http status of the error code.
 *
 * @param error The code to get the status for.
 * @return std::string The error status.
 */
std::string IotServer::http_err_status(httpd_err_code_t error)
{
    switch (error) {
        case HTTPD_501_METHOD_NOT_IMPLEMENTED:
            return "501 Method Not Implemented";
        case HTTPD_505_VERSION_NOT_SUPPORTED:
            return "505 Version Not Supported";
        case HTTPD_400_BAD_REQUEST:
            return "400 Bad Request";
        case HTTPD_401_UNAUTHORIZED:
            return "401 Unauthorized";
        case HTTPD_403_FORBIDDEN:
            return "403 Forbidden";
        case HTTPD_404_NOT_FOUND:
            return "404 Not Found";
        case HTTPD_405_METHOD_NOT_ALLOWED:
            return "405 Method Not Allowed";
        case HTTPD_408_REQ_TIMEOUT:
            return "408 Request Timeout";
        case HTTPD_414_URI_TOO_LONG:
            return "414 URI Too Long";
        case HTTPD_411_LENGTH_REQUIRED:
            return "411 Length Required";
        case HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE:
            return "431 Request Header Fields Too Large";
        case HTTPD_500_INTERNAL_SERVER_ERROR:
        default:
            return "500 Internal Server Error";
    }
}
