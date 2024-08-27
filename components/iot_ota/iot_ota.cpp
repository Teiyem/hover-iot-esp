#include <sys/param.h>
#include "iot_ota.h"
#include "iot_ota.pb-c.h"
#include "iot_application.h"

/** The handle for the OTA update. */
esp_ota_handle_t IotOta::_update_handle{};

/** The http server */
IotServer *IotOta::_iot_server;

/** The next app partition to write new firmware to. */
esp_partition_t const *IotOta::_update_partition;

/** The current ota state. */
iot_ota_state_e IotOta::_ota_state = IOT_OTA_STATE_IDLE;

/** The app description of the current app partition. */
esp_app_desc_t *IotOta::_app_info{nullptr};

/**
 * Initialises a new instance of the IotOta class.
 */
IotOta::IotOta(void)
{
    _iot_server = &IotFactory::create_component<IotServer>();
}

/**
 * Destructor for IotOta class.
 */
IotOta::~IotOta(void)
{
    delete _iot_server;
}

/**
 * Initializes the IotOta component.
 *
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotOta::init(void)
{
    esp_err_t ret = ESP_FAIL;

    _update_partition = esp_ota_get_next_update_partition(nullptr);

    if (_update_partition == nullptr)
    {
        ESP_LOGE(TAG, "%s: Failed to get next ota partition", __func__);
        return ret;
    }

    ret = _iot_server->register_route("update", HTTP_POST, on_update);
    ret |= _iot_server->register_route("update", HTTP_GET, on_status);

    if (ret != ESP_OK)
        ESP_LOGE(TAG, "%s: Failed to register ota route. Error [reason: %s]", __func__, esp_err_to_name(ret));

    return ret;
}

/**
 * Callback function to perform the ota update.
 *
 * @param[in] req The http request object.
 * @return ESP_OK.
 */
esp_err_t IotOta::on_update(httpd_req_t *req)
{
    char buf[IOT_OTA_MAX_BUFFER_SIZE];

    int recv;
    bool got_start = false;
    char res_message[100];

    IotResponse res = IOT_RESPONSE__INIT;
    res.status = IOT_RESPONSE_STATUS__FAILED;
    res.message = res_message;

    size_t res_size = iot_response__get_packed_size(&res);

    char *out_buf = iot_allocate_mem<char>(res_size);

    size_t rem = req->content_len;

    esp_err_t ret = ESP_FAIL;

    if (out_buf == nullptr)
        return _iot_server->send_err(req, iot_error_create_text(HTTPD_500_INTERNAL_SERVER_ERROR,
                                                                "Failed to create response object"));

    while(rem > 0)
    {
        recv = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

        if (recv < 0)
        {
            if (recv == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGI(TAG, "%s: Socket Timed out, retrying to receive content....", __func__ );
                continue;
            }
            ESP_LOGI(TAG, "%s: Failed to receive content [reason: %d]", __func__ , recv);

            res.message = iot_char_s("Failed to receive content");
            iot_response__pack(&res, (uint8_t *) out_buf);
            return ESP_FAIL;
        }

        if (!got_start)
        {
            got_start = true;

            char *temp = strstr(buf, "\r\n\r\n") + 4;
            int temp_len = recv - (temp - buf);

            ESP_LOGI(TAG, "%s: OTA file [size: %d\r\n", __func__ , rem);

            esp_err_t err = start();

            if (err != ESP_OK)
                return _iot_server->send_err(req, iot_error_response_create_proto(HTTPD_500_INTERNAL_SERVER_ERROR, out_buf));

            write(temp, temp_len, &rem);
        }
        else
            write(buf, recv, &rem);
    }

    if (end() != ESP_OK)
    {
        _ota_state = IOT_OTA_STATE_FAILED;
        iot_response__pack(&res, (uint8_t *) out_buf);
        return _iot_server->send_err(req, iot_error_response_create_proto(HTTPD_500_INTERNAL_SERVER_ERROR, out_buf));
    }

    if (_ota_state == IOT_OTA_STATE_SUCCESS)
        IotApplication::reboot(iot_convert_time_to_ms("10s"));

    iot_response__pack(&res, (uint8_t *) out_buf);
    ret = _iot_server->send_res(req, iot_response_create_proto(reinterpret_cast<char *>(&res), res_size));

    free(out_buf);

    return ret;
}

/**
 * Callback function to get the status of the update.
 *
 * @param[in] req The http request object.
 * @return ESP_OK.
 */
esp_err_t IotOta::on_status(httpd_req_t *req)
{
    ESP_LOGI(TAG, "%s: Processing request to get ota status", __func__ );
    IotOtaStatusResponse  res = IOT_OTA_STATUS_RESPONSE__INIT;

    switch (_ota_state) {
        case IOT_OTA_STATE_SUCCESS:
            res.status = IOT_RESPONSE_STATUS__SUCCESS;
            break;
        case IOT_OTA_STATE_FAILED:
            res.status = IOT_RESPONSE_STATUS__FAILED;
            break;
        case IOT_OTA_STATE_REJECTED:
            res.status = IOT_RESPONSE_STATUS__REJECTED;
            break;
        default:
            break;
    }

    if (_app_info != nullptr)
    {
        res.version = _app_info->version;
        res.compile_date = _app_info->date;
        res.compile_time = _app_info->time;
    }
    else
    {
        res.version = iot_char_s("Unknown");
        res.compile_date = iot_char_s(__DATE__);
        res.compile_time = iot_char_s(__TIME__);
    }

    size_t res_size = iot_ota_status_response__get_packed_size(&res);

    char *buf = iot_allocate_mem<char>(res_size);

    if (buf == nullptr)
       return _iot_server->send_err(req, iot_error_create_text(HTTPD_500_INTERNAL_SERVER_ERROR,
                                                         "Failed to serialize response. Out of memory"));

    iot_ota_status_response__pack(&res, reinterpret_cast<uint8_t *>(buf));



    return _iot_server->send_res(req, iot_response_create_proto(buf, res_size));
}

/**
 * Starts the ota update.
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotOta::start(void)
{
    ESP_LOGI(TAG, "%s: Starting update [time: %s]", __func__, iot_now_str());

    esp_err_t ret = esp_ota_begin(_update_partition, OTA_SIZE_UNKNOWN, &_update_handle);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s-> Failed to begin ota [reason: %s]", __func__, esp_err_to_name(ret));
        esp_ota_abort(_update_handle);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s-> Writing to ota partition [subtype: %d, offset: 0x%lx]", __func__, _update_partition->subtype,
             _update_partition->address);

    return  ESP_OK;
}

/**
 * Writes the update to the update partition.
 *
 * @param[in] buf The buffer containing the update to write.
 * @param[in] size The size of the data to write.
 * @param[out] remaining A pointer to the remaining size of the written update in bytes.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotOta::write(char *buf, size_t size, size_t *remaining)
{
    ESP_LOGI(TAG, "%s: Writing [next: %u bytes, remaining: %u bytes]", __func__ , size, *remaining);

    esp_err_t ret = esp_ota_write(_update_handle, buf, size);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Failed to write to ota partition [reason: %s], aborting update", __func__, esp_err_to_name(ret));
        esp_ota_abort(_update_handle);
        return ret;
    }

    *remaining -= size;
    return ret;
}

/**
 * Ends the ota update.
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotOta::end(void)
{
    ESP_LOGI(TAG, "%s: Completing ota update......", __func__);

    esp_err_t ret = esp_ota_end(_update_handle);

    if (ret == ESP_OK)
    {
        ret = esp_ota_set_boot_partition(_update_partition);

        if (ret == ESP_OK)
        {
            const esp_partition_t *current_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "%s: Update successful. Current boot partition [subtype: %d, offset: 0x%lx]", __func__,
                     current_partition->subtype, current_partition->address);

            esp_app_desc_t *app_info = iot_allocate_mem<esp_app_desc_t>(sizeof(esp_app_desc_t));

            if (esp_ota_get_partition_description(current_partition, app_info) == ESP_OK) {
                _app_info = app_info;
            }

            _ota_state = IOT_OTA_STATE_SUCCESS;
        }
        else
            ESP_LOGE(TAG, "%s: Failed to set update [reason: %s]", __func__, esp_err_to_name(ret));
    }
    else
        ESP_LOGE(TAG, "%s: Failed to complete update [reason: %s]", __func__, esp_err_to_name(ret));

    return ret;
}