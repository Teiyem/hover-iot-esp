#include "iot_application.h"

/** Whether this is the first network connection. */
bool IotApplication::_first_connection{true};

/** The current application state. */
iot_app_state_e IotApplication::_app_state{iot_app_state_e::INITIAL};

/** The component used for provisioning.*/
IotProvision *IotApplication::iot_provision{nullptr};

/** The device component.*/
IotDevice *IotApplication::_iot_device{nullptr};

/** The semaphore used to safe guard calls to reboot. */
SemaphoreHandle_t IotApplication::_semaphore{nullptr};

/** The application component's queue handle. */
QueueHandle_t IotApplication::_queue_handle{};

/** The milliseconds used to delay the restart of the device. */
uint64_t IotApplication::_restart_delay{iot_convert_time_to_ms("2s")};

/** The application timezone .*/
char *IotApplication::_timezone{iot_char_s("GMT-2")};

/**
 * Initialises a new instance of the IotApplication class.
 */
IotApplication::IotApplication(void)
{
    set_default_log_levels();
    _iot_wifi = new IotWifi();
    _iot_device = new IotDevice();
    _iot_status_led = &IotFactory::create_component<IotStatusLed>(GPIO_NUM_2, true);

    _components.push_back(_iot_wifi);
    _components.push_back(_iot_status_led);

    _app_desc = esp_app_get_description();
    assert(_app_desc != nullptr);
}

/**
 * Destructor for IotApplication class.
 */
IotApplication::~IotApplication(void)
{
    delete _iot_wifi;
    delete iot_provision;
    delete _iot_device;
    delete _iot_ota;
    vSemaphoreDelete(_semaphore);
}

/**
 * Starts the application component.
 *
 * @param[in] config The application configuration.
 */
void IotApplication::start(iot_app_cfg_t config)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "%s: Starting component", __func__);

    esp_err_t ret = nvs_flash_init();

    _iot_status_led->start();
    _iot_status_led->toggle(true);
    _iot_status_led->set_mode(IOT_SLOW_BLINK);

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    _queue_handle = xQueueCreate(10, sizeof(iot_app_message_e));

    if (config.device_cfg == nullptr) {
        ESP_LOGE(TAG, "%s: Device configuration is required ...aborting", __func__);
        reboot(0);
        return;
    }

    _semaphore = xSemaphoreCreateBinary();

    if (_semaphore == nullptr) {
        ESP_LOGE(TAG, "%s: Failed to create semaphore out of memory ...aborting", __func__);
        reboot(0);
        return;
    }

    _iot_wifi->set_callback([this](iot_wifi_message_e msg) {
        on_wifi_event(msg);
    });

    _iot_wifi->start();

    if (!_iot_wifi->configured()) {
        ESP_LOGI(TAG, "%s: WiFi is not configured yet.", __func__);
        _app_state = iot_app_state_e::CONFIGURING;

        iot_provision = new IotProvision();
        iot_provision->set_callback([this](iot_prov_message_e msg) {
            on_prov_event(msg);
        });

        iot_provision->start();
    } else {
        iot_zero_mem(&_device_data, sizeof(iot_device_data_t));

        auto storage = IotFactory::create_scoped<IotStorage>(IOT_NVS_DEFAULT_PART_NAME, IOT_NVS_DEFAULT_NAMESPACE);

        ret = storage->read(IOT_NVS_DEVICE_DATA_KEY, &_device_data, sizeof(iot_device_data_t));

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "%s: Failed to load application data [reason: %s]", __func__, esp_err_to_name(ret));

            strncpy(_device_data.name, strcat(iot_char_s(("hover.")), _iot_wifi->get_mac()), IOT_MAX_ANY_NAME_LEN);
            strcpy(_device_data.uuid, "");
        }

        _init(config);
    }

    xTaskCreatePinnedToCore(&task, "iot_app_task", 6096, this, 4, nullptr, 1);

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);
}

/**
 * Initializes the rest of the core components.
 *
 * @param config The application configuration.
 */
void IotApplication::_init(iot_app_cfg_t config)
{
    auto server = &IotFactory::create_component<IotServer>();
    server->start();

    char *mac_address = _iot_wifi->get_mac();

    server->register_route("reboot", HTTP_GET, [](httpd_req_t *req) -> esp_err_t {
        httpd_resp_sendstr(req, "Rebooting in 5 seconds");
        IotApp.reboot(iot_convert_time_to_ms("5s"));
        return ESP_OK;
    });

    _iot_wifi->init_mdns(_device_data.name);

    server->set_auth(_device_data.uuid);

    iot_device_meta_t meta = iot_device_meta_t(mac_address, config.model, _app_desc->version,
                                               _app_desc->date);

    iot_device_cfg_t *device = config.device_cfg;

    device->device_info->device_name = _device_data.name;
    device->device_info->metadata = meta;

    for (const auto &service: device->device_info->services) {
        if (service.name == IOT_DEVICE_OTA_SERVICE) {
            if (service.enabled) {
                _iot_ota = new IotOta();
                _iot_ota->init();
            }
        }
    }

    _iot_device->init(config.device_cfg);
}

/**
 * Sets the device to reboot.
 *
 * @param[in] delay The delay in milliseconds before the device reboots.
 */
void IotApplication::reboot(uint64_t delay)
{
    xSemaphoreTake(_semaphore, portMAX_DELAY);

    ESP_LOGI(TAG, "%s: Request to reboot in [time: %llu]", __func__, delay);

    if (_app_state == iot_app_state_e::REBOOTING)
        ESP_LOGW(TAG, "%s: The application is in a reboot state already.", __func__);
    else {
        _restart_delay = delay;
        _app_state = iot_app_state_e::REBOOTING;
    }

    xSemaphoreGive(_semaphore);
}

/**
 * Sets the default log levels for esp specific tags and application specific tags
 */
void IotApplication::set_default_log_levels(void)
{
#if CONFIG_IOT_HOVER_ENV_PROD
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("Iot", ESP_LOG_INFO);
#elif CONFIG_IOT_HOVER_ENV_DEV
    esp_log_level_set("wifi", ESP_LOG_INFO);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_INFO);
    esp_log_level_set("mdns", ESP_LOG_INFO);
    esp_log_level_set("event:", ESP_LOG_INFO);
    esp_log_level_set("Iot", ESP_LOG_VERBOSE);
#endif
}

/**
 * Callback handler for iot wifi events.
 *
 * @param[in] msg The wifi event message.
 */
void IotApplication::on_wifi_event(iot_wifi_message_e msg)
{
    switch (msg) {
        case IOT_WIFI_MSG_CONNECTED:
            send_to_queue(IOT_APP_MSG_WIFI_CONNECT_OK);
            break;
        case IOT_WIFI_MSG_DISCONNECTED:
            send_to_queue(IOT_APP_MSG_WIFI_DISCONNECT);
            break;
        case IOT_WIFI_MSG_CONNECT_FAILED:
            send_to_queue(IOT_APP_MSG_WIFI_DISCONNECT);
            break;
        case IOT_WIFI_MSG_RECONNECTING:
            send_to_queue(IOT_APP_MSG_WIFI_DISCONNECT);
            break;
        case IOT_WIFI_MSG_RECONNECTING_FAIL:
            send_to_queue(IOT_APP_MSG_WIFI_DISCONNECT);
            break;
        default:
            break;
    }
}

/**
 * Callback handler for iot provision events.
 *
 * @param[in] msg The provision event message.
 */
void IotApplication::on_prov_event(iot_prov_message_e msg)
{
    switch (msg) {

        case IOT_PROV_MSG_STARTED:
            send_to_queue(IOT_APP_MSG_PROV_START);
            break;
        case IOT_PROV_MSG_FINISHED:
            send_to_queue(IOT_APP_MSG_PROV_OK);
            break;
        case IOT_PROV_MSG_FAIL:
            send_to_queue(IOT_APP_MSG_PROV_FAIL);
            break;
        default:
            break;
    }
}

/**
 * Initializes the timezone and the sntp time server.
 *
 * @param time_zone The timezone of the device,
 */
void IotApplication::init_sntp(char *timezone)
{
    ESP_LOGI(TAG, "%s: Initializing sntp with [timezone: %s]", __func__, timezone);

    setenv("TZ", timezone, 1);
    tzset();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.google.com");
    sntp_setservername(1, "pool.ntp.com");
    sntp_set_time_sync_notification_cb(on_sntp_update);
    sntp_set_sync_interval(_clock_sync_time);

    sntp_init();

    ESP_LOGI(TAG, "%s: Done initializing sntp", __func__);
}

/**
 * Callback handler for time synchronization notifications, Sets the device's time to.
 *
 * @param[in] tv A pointer to the time received from the server.
 */
void IotApplication::on_sntp_update(timeval *tv)
{
    ESP_LOGI(TAG, "%s: Current [time: %s]", __func__, iot_now_str());
    settimeofday(tv, nullptr);
    sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}

/**
 * Sends a message to the queue.
 *
 * @param[in] msg The message to send.
 * @return pdTrue on success, otherwise pdFALSE.
 */
BaseType_t IotApplication::send_to_queue(iot_app_message_e msg)
{
    return xQueueSend(_queue_handle, &msg, portMAX_DELAY);
}

/**
 * Task for the IotApplication component.
 *
 * @param[in] param A pointer to the task parameter (this).
 */
IRAM_ATTR [[noreturn]] void IotApplication::task(void *param)
{
    size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "%s -> Task started running, current total heap -> %u", __func__, heap_size);

    auto *self = static_cast<IotApplication *>(param);

    if (self == nullptr)
        esp_system_abort("Pointer to iot app is null, Did you forget to pass it as a param to the task ?");

    iot_app_message_e msg;

    while (true) {
        size_t heap_free_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);

        size_t heap_usage = heap_size - heap_free_size;

        ESP_LOGI(TAG, "%s -> Task heap usage -> %u bytes", __func__, heap_usage);

        if (_app_state == iot_app_state_e::REBOOTING) {
            ESP_LOGI(TAG, "%s: Rebooting device............", __func__);

            for (const auto &component: self->_components) {
                component->stop();
            }

            vTaskDelay(_restart_delay / portTICK_PERIOD_MS);
            esp_restart();
        }

        if (xQueueReceive(_queue_handle, &msg, portMAX_DELAY) == pdTRUE)
            self->process_message(msg);
    }
}

/**
 * Processes an iot application message.
 *
 * @param[in] message The message to process.
 */
void IotApplication::process_message(iot_base_message_e message)
{
    iot_app_message_e msg_id = static_cast<iot_app_message_e>(message);

    ESP_LOGI(TAG, "%s: Processing message [id: %u]", __func__, msg_id);

    switch (msg_id) {
        case IOT_APP_MSG_WIFI_CONNECT_OK:
            if (_app_state == iot_app_state_e::CONFIGURING)
                return;
            on_connected();
            break;
        case IOT_APP_MSG_WIFI_CONNECT_FAIL:
            if (_app_state == iot_app_state_e::CONFIGURING) {
                ESP_LOGI(TAG, "%s: Failed to connect, restarting", __func__);
                reboot(0);
            }
            break;
        case IOT_APP_MSG_WIFI_RECONNECT:
            _app_state = iot_app_state_e::CONNECTING;
            break;
        case IOT_APP_MSG_WIFI_RECONNECT_OK:
            on_connected();
            break;
        case IOT_APP_MSG_WIFI_RECONNECT_FAIL:
            reboot(0);
            break;
        case IOT_APP_MSG_WIFI_DISCONNECT:
            _iot_status_led->set_mode(IOT_SLOW_BLINK);
            _app_state = iot_app_state_e::CONNECTING;
            break;
        case IOT_APP_MSG_PROV_START:
            _iot_status_led->set_mode(IOT_SLOW_BLINK);
            break;
        case IOT_APP_MSG_PROV_OK:
            _app_state = iot_app_state_e::CONFIGURED;
            _iot_status_led->set_mode(IOT_STATIC);
            reboot(iot_convert_time_to_ms("10s"));
            break;
        case IOT_APP_MSG_PROV_FAIL:

            break;
        case IOT_APP_MSG_OTA_UPDATE_OK:

            break;
        case IOT_APP_MSG_OTA_UPDATE_FAIL:

            break;
        default:
            break;
    }
}

/**
 * Handles network connected.
 */
void IotApplication::on_connected()
{
    if (!_iot_status_led->state())
        _iot_status_led->toggle(true);

    _iot_status_led->set_mode(IOT_STATIC);

    if (_first_connection) {
        init_sntp(_timezone);

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
        _iot_mqtt = &IotFactory::create_component<IotMqtt>();
        _iot_mqtt->start(std::string(_device_data.name) + "_" + std::string(_iot_wifi->get_mac()));
#endif
        _first_connection = false;
    }

    if (_app_state != iot_app_state_e::REBOOTING) {
        _app_state = iot_app_state_e::CONNECTED;

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
        if(_iot_mqtt != nullptr && !_iot_mqtt->is_connected())
            _iot_mqtt->reconnect();
#endif
    }
}

IotApplication IotApp;