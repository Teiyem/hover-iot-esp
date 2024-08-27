#include "iot_provision.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "qrcode.h"
#include <cJSON.h>
#include "iot_provision.pb-c.h"

/** The storage component.*/
IotStorage *IotProvision::_iot_storage{};

/** The wifi data received from the provisioning. */
iot_wifi_data_t IotProvision::_wifi_data{};

/** A callback function to call when provision events occurs. */
std::function<void(iot_prov_message_e)> IotProvision::_evt_cb{nullptr};

/** A handle to the queue used to send messages from events to the task. */
QueueHandle_t IotProvision::_queue_handle{nullptr};

/** A handle to the task. */
TaskHandle_t IotProvision::_task_handle{};

/**
 * Initialises a new instance of the IotProvision class.
 */
IotProvision::IotProvision(void)
{
    _iot_storage = new IotStorage(IOT_NVS_DEFAULT_PART_NAME, IOT_NVS_DEFAULT_NAMESPACE);
}

/**
 * Sets a callback function to be called when certain setup events occur.
 *
 * @param[in] cb The callback function to call when an event occurs.
 */
void IotProvision::set_callback(std::function<void(iot_prov_message_e)> evt_cb)
{
    assert(evt_cb != nullptr);
    _evt_cb = evt_cb;
}

/**
 * Starts the component.
 */
void IotProvision::start(void)
{
    ESP_LOGI(TAG, "%s: Starting component", __func__);

    assert(_evt_cb != nullptr);

    _queue_handle = xQueueCreate(3, sizeof(iot_prov_message_e));

    init();

    xTaskCreatePinnedToCore(&runner, "iot_provision_task", 4096, this, 5, &_task_handle, 0);

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);
}

/**
 * Initializes wifi provision and starts the provision config process.
 */
void IotProvision::init(void)
{
    ESP_LOGI(TAG, "%s: Initializing provision", __func__);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                               &on_event, this));

    auto storage = new IotStorage(IOT_NVS_FACTORY_PART_NAME, IOT_NVS_FACTORY_NAMESPACE);
    size_t salt_len = 0;
    char *salt = nullptr;

    esp_err_t ret = storage->read("prov", reinterpret_cast<void **>(&salt), salt_len, TYPE_BLOB);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to get setup salt [reason: %s]", __func__, esp_err_to_name(ret));
        send_to_queue(IOT_PROV_MSG_FAIL);
        return;
    }

    size_t verifier_len = 0;
    char *verifier = nullptr;

    ret = storage->read("prov", reinterpret_cast<void **>(&verifier), verifier_len, TYPE_BLOB);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to get verifier salt [reason: %s]", __func__, esp_err_to_name(ret));
        send_to_queue(IOT_PROV_MSG_FAIL);
        return;
    }

    const wifi_prov_mgr_config_t config = {
            .scheme = wifi_prov_scheme_softap,
            .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
            .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    char service_name[12];

    create_service_name(service_name);

    wifi_prov_security_t security = WIFI_PROV_SECURITY_2;

    wifi_prov_security2_params_t sec_params = {
            .salt = salt,
            .salt_len = static_cast<uint16_t>(salt_len),
            .verifier = verifier,
            .verifier_len = static_cast<uint16_t>(verifier_len)
    };

    const char *service_key = "12345678";

    wifi_prov_mgr_endpoint_create("provision-data");

    const void *sec_params_ptr = static_cast<const void *>(&sec_params);

    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, sec_params_ptr, service_name, service_key));

    wifi_prov_mgr_endpoint_register("provision-data", on_data, nullptr);

    print_qrcode(service_name, "iot-prov", "abcd1234", service_key);

    ESP_LOGI(TAG, "%s: Starting provision", __func__);
}

/**
 * Prints a qrcode to the logs.
 *
 * @param[in] name The service name.
 * @param[in] username The service user name.
 * @param[in] pop The service proof of possession.
 * @param[in] key The service key.
 */
void IotProvision::print_qrcode(const char *name, const char *username, const char *pop, const char *key)
{
    auto json = cJSON_CreateObject();

    if (json == nullptr)
        ESP_LOGE(TAG, "%s: Failed to create json object [reason: %s]", __func__, cJSON_GetErrorPtr());

    cJSON_AddStringToObject(json, "ver", IOT_PROV_QR_VERSION);
    cJSON_AddStringToObject(json, "name", name);
    cJSON_AddStringToObject(json, "username", username);
    cJSON_AddStringToObject(json, "pop", pop);
    cJSON_AddStringToObject(json, "transport", "softap");
    cJSON_AddStringToObject(json, "password", key);

    const char *payload = cJSON_Print(json);

    ESP_LOGI(TAG, "Scan this QR code from the provisioning application for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);

    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", IOT_PROV_QRCODE_URL,
             payload);
}

/**
 * Handles Wi-Fi and Wi-Fi provision related events.
 *
 * @param[in] args A pointer to the user data.
 * @param[in] base The event base for the handler.
 * @param[in] id The id of the received event.
 * @param[in] data A pointer to the event data.
 */
void IotProvision::on_event(void *args, const esp_event_base_t base, int32_t id, void *data)
{
    static int retries;

    if (base == WIFI_PROV_EVENT) {
        switch (id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "%s: Received [id: WIFI_PROV_START]", __func__);
                send_to_queue(IOT_PROV_MSG_STARTED);
                break;
            case WIFI_PROV_CRED_RECV: {
                const wifi_sta_config_t *wifi_sta_cfg = static_cast<wifi_sta_config_t *>(data);
                ESP_LOGI(TAG, "%s: Received [id: WIFI_PROV_CRED_RECV] "
                              "\n\t\tSSID: %s"
                              "\n\t\tPassword: %s", __func__, iot_mask_str((const char *) (wifi_sta_cfg->ssid)),
                         iot_mask_str((const char *) (wifi_sta_cfg->password)));
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                const wifi_prov_sta_fail_reason_t *reason = static_cast<wifi_prov_sta_fail_reason_t *>(data);

                ESP_LOGE(TAG, "%s: Received [id: WIFI_PROV_CRED_FAIL, reason: %s]", __func__,
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");

                retries++;

                if (retries >= 10) {
                    ESP_LOGI(TAG, "Failed to connect with provisioned AP, resetting provisioned credentials");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    retries = 0;
                }
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "%s: Received event_id -> Provisioning successful", __func__);
                retries = 0;
                send_to_queue(IOT_PROV_MSG_SUCCESS);
                break;
            case WIFI_PROV_END:
                ESP_LOGI(TAG, "%s: Received event_id -> Provisioning end", __func__);
                wifi_prov_mgr_deinit();
                send_to_queue(IOT_PROV_MSG_FINISHED);
                break;
            default:
                break;
        }
    } else if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "%s: Received event [id: WIFI_EVENT_STA_START].", __func__);
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "%s: Received event [id: WIFI_EVENT_STA_DISCONNECTED], Connecting to the network again...", __func__);
                esp_wifi_connect();
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "%s: Received event [id: WIFI_EVENT_AP_STACONNECTED].", __func__);
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "%s: Received event [id: WIFI_EVENT_AP_STADISCONNECTED].", __func__);
                break;
            default:
                break;
        }
    }
}

/**
 * Handles the provisioning data.
 *
 * @param[in] session The Unique session identifier.
 * @param[in] inbuf A pointer to the input data.
 * @param[in] inlen The length of the input data.
 * @param[out] outbuf A pointer to the output data.
 * @param[out] outlen A pointer to the length of the output data.
 * @param[in] priv_data A pointer to the private data.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotProvision::on_data(uint32_t session, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen,
                      [[maybe_unused]] void *priv_data)
{
    if (inbuf == nullptr)
        return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "%s: Received from client [size: %zd , data: %s]", __func__, inlen, (char *) inbuf);

    auto *data = iot_provision_request__unpack(nullptr, inlen, inbuf);

    if (data == nullptr)
        return ESP_ERR_NO_MEM;

    if (!iot_valid_str(data->server_url) && !iot_valid_str(data->device_name) &&
        !iot_valid_str(data->device_uuid) && !iot_valid_str(""))
        return ESP_FAIL;

    iot_device_data_t device_data;
    iot_zero_mem(&device_data, sizeof(iot_device_data_t));

    strcpy(device_data.server_url, data->server_url);
    strcpy(device_data.name, data->device_name);
    strcpy(device_data.uuid, data->device_uuid);
    strcpy(device_data.timezone, data->timezone);

    if (save(device_data) != ESP_OK) {
        iot_provision_request__free_unpacked(data, nullptr);
        return ESP_FAIL;
    }

    iot_provision_request__free_unpacked(data, nullptr);

    IotProvisionResponse res = IOT_PROVISION_RESPONSE__INIT;
    res.status = IOT_PROVISION_STATUS__Success;
    res.message = (char *) "successfully saved setup data";

    size_t res_size = iot_provision_response__get_packed_size(&res);

    *outbuf = iot_allocate_mem<uint8_t>(res_size);

    if (*outbuf == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    iot_provision_response__pack(&res, *outbuf);

    *outlen = res_size;

    return ESP_OK;
}

/**
 * Generates a unique service name for the device based on its MAC address.
 *
 * @param service_name The service name.
 */
void IotProvision::create_service_name(char *service_name)
{
    uint8_t mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(service_name, 12, "%s%02X%02X%02X", ssid_prefix, mac[3], mac[4], mac[5]);
}

/**
 * Saves the device data.
 *
 * @param[in] device_data The device data to save.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotProvision::save(iot_device_data_t device_data)
{
    ESP_LOGI(TAG, "%s: Writing device data to storage", __func__);

    const iot_nvs_write_params_t write_params = iot_nvs_write_params_t(IOT_NVS_DEVICE_DATA_KEY,
                                                                       &device_data, sizeof(iot_device_data_t));

    esp_err_t ret = _iot_storage->write(&write_params);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to write device data to storage", __func__);
        return ret;
    }

    return ESP_OK;
}

/**
 * Sends a message to the queue.
 *
 * @param[in] msg The message to send.
 * @returns pdTrue on success, otherwise pdFALSE.
 */
BaseType_t IotProvision::send_to_queue(iot_prov_message_e msg)
{
    return xQueueSend(_queue_handle, &msg, portMAX_DELAY);
}

/**
 * Task for the IotProvision component.
 *
 * @param[in] param A pointer to the task parameter (this).
 */
[[noreturn]] void IotProvision::runner(void *param)
{
    ESP_LOGI(TAG, "%s -> Task started running", __func__);

    auto *self = static_cast<IotProvision *>(param);

    iot_not_null(self);

    iot_prov_message_e msg;

    while (true)
    {
        if (xQueueReceive(_queue_handle, &msg, portMAX_DELAY) == pdTRUE)
            self->process_message(msg);
    }
}

/**
 * Processes the messages in the queue.
 *
 * @param[in] msg The message to process.
 */
void IotProvision::process_message(iot_prov_message_e msg)
{
    ESP_LOGI(TAG, "%s: Processing message with id -> %d", __func__, msg);

    switch (msg) {
        case IOT_PROV_MSG_SUCCESS: {
            wifi_config_t current_cfg = {};
            ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &current_cfg));

            iot_zero_mem(&_wifi_data, sizeof(iot_wifi_data_t));

            memcpy(_wifi_data.ssid, current_cfg.sta.ssid,
                   std::min(sizeof(iot_wifi_data_t::ssid), sizeof(wifi_sta_config_t::ssid)));
            memcpy(_wifi_data.password, iot_mask_str(reinterpret_cast<const char *>(current_cfg.sta.password)),
                   std::min(sizeof(iot_wifi_data_t::password), sizeof(wifi_sta_config_t::password)));

            const iot_nvs_write_params_t write_params = iot_nvs_write_params_t(IOT_NVS_WIFI_DATA_KEY,
                                                                               &_wifi_data, sizeof(iot_wifi_data_t));

            esp_err_t ret = _iot_storage->write(&write_params);

            if (ret != ESP_OK)
                ESP_LOGI(TAG, "%s: Failed to write wifi data [reason: %s]", __func__, esp_err_to_name(ret));
        }
            break;
        case IOT_PROV_MSG_FINISHED:
        default:
            _evt_cb(msg);
            break;
    }
}
