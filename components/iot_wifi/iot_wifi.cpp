#include "iot_wifi.h"
#include "mdns.h"

#define IOT_MDNS_INSTANCE "mdns-iot-hover"

/** The device's mac address. */
char IotWifi::_mac[]{};

/** Indicates whether the Wi-Fi is configured. */
bool IotWifi::_configuring{false};

/** Indicates whether the device is connected to the network. */
bool IotWifi::_connected{false};

/** Indicates whether to attempt to reconnect to the network. */
bool IotWifi::_reconnect{false};

/** Indicates whether a reconnection attempt is in progress. */
bool IotWifi::_reconnecting{false};

/** The number of Wi-Fi reconnect attempts. */
uint8_t IotWifi::_retries{0};

/** The configuration for Wi-Fi initialization. */
wifi_init_config_t IotWifi::_wifi_config = WIFI_INIT_CONFIG_DEFAULT();

/** A handle to the queue used to send messages from events to the task. */
QueueHandle_t IotWifi::_queue_handle{};

/** A handle to the task. */
TaskHandle_t IotWifi::_task_handle{};

/** A callback handler to call when Wi-Fi events occurs. */
std::function<void(iot_wifi_message_e)> IotWifi::_evt_cb{};

/**
 * Initialises a new instance of the IotWifi class.
 */
IotWifi::IotWifi()
{
    _iot_storage = new IotStorage(IOT_NVS_DEFAULT_PART_NAME, IOT_NVS_DEFAULT_NAMESPACE);
}

/**
 * Destructor for IotWifi class.
 */
IotWifi::~IotWifi()
{
    delete _iot_storage;
    mdns_free();
    netbiosns_stop();
}

/**
 * Starts the component.
 */
void IotWifi::start()
{
    ESP_LOGI(TAG, "%s: Starting component", __func__);

    assert(_evt_cb != nullptr);

    _queue_handle = xQueueCreate(10, sizeof(iot_wifi_message_e));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_event, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_event, this, nullptr));

    init_default_config();

    esp_err_t ret = check_configuration();

    if (ret != ESP_OK) {
        _configuring = true;
        ESP_LOGI(TAG, "%s: Couldn't load credentials [reason: %s ]", __func__, esp_err_to_name(ret));
    }

    wifi_mode_t mode = _configuring ? WIFI_MODE_APSTA : WIFI_MODE_STA;

    if (_configuring)
        esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_start());

    read_mac();

    xTaskCreatePinnedToCore(&runner, "iot_wifi_task", 4096, this, 5,
                            &_task_handle, 0);

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);
}

/**
 * Stops the component.
 */
void IotWifi::stop()
{
    ESP_LOGI(TAG, "%s: Stopping component", __func__);

    mdns_free();
    netbiosns_stop();
    vTaskDelete(_task_handle);
    vQueueDelete(_queue_handle);
    _queue_handle = nullptr;
    _task_handle = nullptr;
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_event);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_event);
    delete _iot_storage;
}

/**
 * Initializes mDNS.
 *
 * @param[in] device_name The hostname for the device.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotWifi::init_mdns(std::string device_name)
{
    esp_err_t ret = mdns_init();

    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "%s: Failed to initialize mdns [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    std::transform(device_name.begin(), device_name.end(), device_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::replace(device_name.begin(), device_name.end(), ' ', '-');

    ESP_LOGI(TAG, "%s: Using transformed [name: %s] for the device name", __func__, device_name.c_str());

    ret = mdns_hostname_set(device_name.c_str());

    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "%s: Failed to set mdns hostname [reason: %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_instance_name_set(IOT_MDNS_INSTANCE);

    if (ret != ESP_OK)
        ESP_LOGI(TAG, "%s: Failed to set mdns instance name [reason: %s]", __func__, esp_err_to_name(ret));

    netbiosns_init();
    netbiosns_set_name(device_name.c_str());

    return ret;
}

/**
 * Sets a callback function to be called when certain wifi events occur.
 *
 * @param[in] cb A pointer to the callback function to call.
 */
void IotWifi::set_callback(std::function<void(iot_wifi_message_e)> evt_cb)
{
    _evt_cb = evt_cb;
}

/**
 * Check if the WiFi is currently connected.
 *
 * @return true if connected, false otherwise.
 */
[[maybe_unused]] bool IotWifi::connected()
{
    return _connected;
}

/**
 * Check if the WiFi has been configured.
 *
 * @return true if configured, false if currently configuring.
 */
bool IotWifi::configured()
{
    return !_configuring;
}

/**
 * Gets the mac address.
 *
 * @return char * The device mac address.
 */
char *IotWifi::get_mac()
{
    return _mac;
}

/**
 * Initializes the TCP stack and default WiFi configurations.
 */
void IotWifi::init_default_config()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_wifi_init(&_wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    esp_netif_create_default_wifi_sta();
}

/**
 * Checks for the wifi credentials in the nvs storage.
 *
 * @return ESP_OK on success, otherwise an error code.
 * @note Assume the wifi isn't configured if not ESP_OK.
 */
esp_err_t IotWifi::check_configuration()
{
    iot_wifi_data_t creds;

    iot_zero_mem(&creds, sizeof(iot_wifi_data_t));

    esp_err_t ret = _iot_storage->read(IOT_NVS_WIFI_DATA_KEY, &creds, sizeof(iot_wifi_data_t));

    if (ret != ESP_OK)
        return ret;

    // Password is masked.
    ESP_LOGI(TAG, "%s: Stored credentials [ssid: %s , password: %s]", __func__, creds.ssid, creds.password);

    return ret;
}

/**
 * Reads the device mac address.
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotWifi::read_mac(void)
{
    ESP_LOGI(TAG, "%s: Getting device mac address", __func__);

    uint8_t mac_byte_buffer[6]{};

    const auto status = esp_efuse_mac_get_default(mac_byte_buffer);

    if (status == ESP_OK) {
        snprintf(_mac, sizeof(_mac), "%02X%02X%02X%02X%02X%02X",
                 mac_byte_buffer[0], mac_byte_buffer[1],
                 mac_byte_buffer[2], mac_byte_buffer[3],
                 mac_byte_buffer[4], mac_byte_buffer[5]);
    }

    ESP_LOGI(TAG, "%s: Retrieved device [mac address: %s]", __func__, _mac);

    return status;
}

/**
 * Attempts to connect to the Wi-Fi.
 */
void IotWifi::connect()
{
    ESP_LOGI(TAG, "%s: Attempting to connect to the wifi", __func__);
    esp_wifi_connect();
}

/**
 * Attempts to reconnect to the Wi-Fi.
 */
void IotWifi::reconnect()
{
    _reconnecting = true;

    if (_retries < _max_retries) {
        vTaskDelay(connect_interval / portTICK_PERIOD_MS);
        connect();
        _retries++;
    } else {
        ESP_LOGI(TAG, "%s: Failed to reconnect to the wifi", __func__);

        if (connect_interval == 50000) {
            _reconnect = false;
            _reconnecting = false;
            send_to_queue(IOT_WIFI_MSG_RECONNECTING_FAIL);
            return;
        }

        connect_interval += 1000;
        _retries = 0;
    }

    _reconnecting = false;
}

/**
 * Handles Wi-Fi related events.
 *
 * @param[in] args A pointer to the user data.
 * @param[in] base The event base for the handler.
 * @param[in] id The id of the received event.
 * @param[in] data A pointer to the event data.
 */
void IotWifi::on_event(void *args, esp_event_base_t base, int32_t id, void *data)
{
    auto *self = static_cast<IotWifi *>(args);

    iot_not_null(self);

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "%s: Received event [id: WIFI_EVENT_STA_START]", __func__);
        send_to_queue(IOT_WIFI_MSG_STARTED);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *event = static_cast<wifi_event_sta_disconnected_t *>(data);
        ESP_LOGI(TAG, "%s: Received event [id: WIFI_EVENT_STA_DISCONNECTED evt_data:[reason: %d ]]", __func__,
                 event->reason);

        if (event->reason == WIFI_REASON_AUTH_FAIL && _configuring)
            return;

        send_to_queue(IOT_WIFI_MSG_DISCONNECTED);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(data);
        ESP_LOGI(TAG, "%s: Received event [id: IP_EVENT_STA_GOT_IP, evt_data:[ip_address: " IPSTR " ]]", __func__,
                 IP2STR(&event->ip_info.ip));
        send_to_queue(IOT_WIFI_MSG_CONNECTED);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP) {
        ESP_LOGI(TAG, "%s: Received event [id: IP_EVENT_STA_LOST_IP ]", __func__);
        send_to_queue(IOT_WIFI_MSG_DISCONNECTED);
    }
}

/**
 * Sends a message to the queue.
 *
 * @param[in] msg The message to send.
 * @returns pdTrue on success, otherwise pdFALSE.
 */
BaseType_t IotWifi::send_to_queue(iot_wifi_message_e msg)
{
    return xQueueSend(_queue_handle, &msg, portMAX_DELAY);
}

/**
 * Task for the IotWifi component.
 *
 * @param[in] param A pointer to the task parameter (this).
 */
[[noreturn]] void IotWifi::runner(void *param)
{
    ESP_LOGI(TAG, "%s -> Task started running", __func__);

    auto *self = static_cast<IotWifi *>(param);

    iot_not_null(self);

    iot_wifi_message_e msg;

    while (true)
    {
        if (xQueueReceive(_queue_handle, &msg, portMAX_DELAY))
            self->process_message(msg);
    }
}

/**
 * Processes the messages in the queue.
 *
 * @param[in] msg The message to process.
 */
void IotWifi::process_message(iot_base_message_e msg)
{
    switch (msg) {
        case IOT_WIFI_MSG_STARTED:
                connect();
            break;

        case IOT_WIFI_MSG_CONNECTED:
            on_state_changed(true);
            break;

        case IOT_WIFI_MSG_DISCONNECTED:
            on_state_changed(false);
            break;

        case IOT_WIFI_MSG_CONNECT_FAILED:
            on_state_changed(false);
            break;

        case IOT_WIFI_MSG_RECONNECTING_FAIL:
            _evt_cb(IOT_WIFI_MSG_RECONNECTING_FAIL);
            break;

        default:
            ESP_LOGW(TAG, "Received unknown message [id: %ld]", msg);
            break;
    }
}

/**
 * Handles changes in network connectivity state.
 *
 * @param[in] connected Whether the network is connected or disconnected.
 */
void IotWifi::on_state_changed(bool connected)
{
    if (connected) {
        _connected = true;
        _retries = 0;
        _reconnect = false;
        _evt_cb(IOT_WIFI_MSG_CONNECTED);
    } else {
        if (_reconnecting)
            return;

        ESP_LOGI(TAG, "%s: Attempting to reconnect to the wifi", __func__);

        if (!_reconnect) {
            _connected = false;
            _reconnect = true;
        }

        reconnect();
        _evt_cb(IOT_WIFI_MSG_RECONNECTING);
    }
}
