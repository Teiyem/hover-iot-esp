#include <cJSON.h>
#include <sys/param.h>
#include "iot_ota.h"

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
 * Initializes the IotOta component.
 * @param app_desc The app description.
 *
 * @return Returns ESP_OK on success, otherwise an error code
 */
esp_err_t IotOta::init(esp_app_desc_t *app_desc)
{
    esp_err_t ret = ESP_FAIL;

    const esp_partition_t *running = esp_ota_get_running_partition();

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "%s: Marking update as success", __func__);
                esp_ota_mark_app_valid_cancel_rollback();
//                esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }

    _app_info = app_desc;

    _update_partition = esp_ota_get_next_update_partition(nullptr);

    if (_update_partition == nullptr) {
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
    bool body_start = false;
    size_t rem = req->content_len;
    esp_err_t ret = ESP_FAIL;
    _ota_state = IOT_OTA_STATE_FAILED;
    int read = 0;

    do {
        recv = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

        if (recv < 0) {
            if (recv == HTTPD_SOCK_ERR_TIMEOUT) {
                ESP_LOGI(TAG, "%s: Socket Timed out, retrying to receive content....", __func__);
                continue;
            }
            ESP_LOGI(TAG, "%s: Failed to receive content [reason: %d]", __func__, recv);

            return _iot_server->send_err(req, "Failed to receive content");
        }

        if (!body_start) {
            body_start = true;

            char *temp = strstr(buf, "\r\n\r\n");

            if (temp == nullptr) {
                ESP_LOGE(TAG, "%s: Malformed request, no header-body separator found", __func__);
                return _iot_server->send_err(req, "Malformed request");
            }

            temp += 4;

            int len = recv - (temp - buf);

            ESP_LOGI(TAG, "%s: OTA file [size: %d\r\n", __func__, rem);

            ret = start();

            if (ret != ESP_OK)
                return _iot_server->send_err(req, "Failed to start update");

            ret = write(temp, len, &rem);

            if (ret != ESP_OK)
                return _iot_server->send_err(req, "Failed to write update");

            read += len;
        } else {
            ret = write(buf, recv, &rem);
            if (ret != ESP_OK)
                return _iot_server->send_err(req, "Failed to write update");

            read += recv;
        }


    } while (recv > 0 && read < req->content_len);

    ret = end();

    if (ret != ESP_OK)
        return _iot_server->send_err(req, "Failed to end update");

    if (_ota_state == IOT_OTA_STATE_SUCCESS) {
        iot_event_request_reboot_t reboot = {};
        esp_event_post(IOT_EVENT, IOT_APP_MSG_OTA_UPDATE_OK, &reboot, sizeof(iot_event_request_reboot_t), portMAX_DELAY);
    }

    ret = _iot_server->send_res(req, "Update completed", true);

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
    ESP_LOGI(TAG, "%s: Processing request to get ota status", __func__);

    cJSON *res = cJSON_CreateObject();

    cJSON_AddStringToObject(res, "version", _app_info->version);
    cJSON_AddStringToObject(res, "compile_date", __DATE__);
    cJSON_AddStringToObject(res, "compile_time", __TIME__);

    const char *status = "status";

    switch (_ota_state) {
        case IOT_OTA_STATE_IDLE:
            cJSON_AddStringToObject(res, status, "idle");
            break;
        case IOT_OTA_STATE_SUCCESS:
            cJSON_AddStringToObject(res, status, "updated");
            break;
        case IOT_OTA_STATE_FAILED:
            cJSON_AddStringToObject(res, status, "failed");
            break;
        case IOT_OTA_STATE_REJECTED:
            cJSON_AddStringToObject(res, status, "error");
            break;
        default:
            break;
    }
    char *buf = cJSON_Print(res);

    cJSON_free(res);

    if (buf == nullptr)
        return _iot_server->send_err(req,IOT_HTTP_SERIALIZATION_ERR);


    return _iot_server->send_res(req, buf);
}

/**
 * Starts the ota update.
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotOta::start(void)
{
    ESP_LOGI(TAG, "%s: Starting update [time: %s]", __func__, iot_now_str().c_str());

    esp_err_t ret = esp_ota_begin(_update_partition, OTA_SIZE_UNKNOWN, &_update_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s-> Failed to begin ota [reason: %s]", __func__, esp_err_to_name(ret));
        esp_ota_abort(_update_handle);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s-> Writing to ota partition [subtype: %d, offset: 0x%lx]", __func__, _update_partition->subtype,
             _update_partition->address);

    return ESP_OK;
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
    ESP_LOGI(TAG, "%s: Writing [next: %u bytes, remaining: %u bytes]", __func__, size, *remaining);

    esp_err_t ret = esp_ota_write(_update_handle, buf, size);

    if (ret != ESP_OK) {
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

    if (ret == ESP_OK) {
        ret = esp_ota_set_boot_partition(_update_partition);

        if (ret == ESP_OK) {
            const esp_partition_t *current_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "%s: Update successful. Current boot partition [subtype: %d, offset: 0x%lx]", __func__,
                     current_partition->subtype, current_partition->address);

            esp_app_desc_t *app_info = iot_allocate_mem<esp_app_desc_t>(sizeof(esp_app_desc_t));

            if (esp_ota_get_partition_description(current_partition, app_info) == ESP_OK) {
                _app_info = app_info;
            }

            _ota_state = IOT_OTA_STATE_SUCCESS;
        } else
            ESP_LOGE(TAG, "%s: Failed to set update [reason: %s]", __func__, esp_err_to_name(ret));
    } else
        ESP_LOGE(TAG, "%s: Failed to complete update [reason: %s]", __func__, esp_err_to_name(ret));

    return ret;
}