#include "iot_application.h"

/** Whether this is the first network connection. */
bool IotApplication::_first_connection{true};

/** The current application state. */
IotAppState IotApplication::_app_state{IotAppState::INITIAL};

/** The component used for provisioning.*/
IotProvision *IotApplication::_iot_provision{nullptr};

/** The device component.*/
IotDevice *IotApplication::_iot_device{nullptr};

/** A handle to the queue used to send messages from events to the task. */
QueueHandle_t IotApplication::_queue_handle{};

/** A handle to lock/unlock the application task . */
SemaphoreHandle_t IotApplication::_task_lock{};

/** The timer used to ensure that the lock is not in a block state for too long. */
TimerHandle_t IotApplication::_lock_timeout{nullptr};

/** The mutex used to safe guard calls to reboot. */
std::mutex IotApplication::_reboot_mutex{};

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
    _iot_wifi = &IotFactory::create_component<IotWifi>();
    _iot_device = &IotFactory::create_component<IotDevice>();
    _iot_status = &IotFactory::create_component<IotStatus>(GPIO_NUM_2, true);

    _components.push_back(_iot_wifi);
    _components.push_back(_iot_status);

    _app_desc = esp_app_get_description();
    assert(_app_desc != nullptr);
}

/**
 * Destroys the IotApplication class.
 */
IotApplication::~IotApplication(void)
{
    stop();
}

/**
 * Starts the application component.
 *
 * @param[in] config The application configuration.
 */
void IotApplication::start(iot_app_cfg_t config)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "%s: Starting component %s", __func__, iot_now_str().data());

    esp_err_t ret = nvs_flash_init();

    _iot_status->start();
    _iot_status->state();
    _iot_status->set_mode(IotLedMode::IOT_LED_SLOW_BLINK);

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    _queue_handle = xQueueCreate(10, sizeof(iot_event_queue_t));
    iot_not_null(_queue_handle);

    _task_lock = xSemaphoreCreateBinary();
    iot_not_null(_task_lock);

    _lock_timeout = xTimerCreate("lock_timeout_timer", pdMS_TO_TICKS(iot_convert_time_to_ms("1m 30s")),
                                 pdFALSE, (void *)0, lock_timeout);

    iot_not_null(_lock_timeout);

    if (config.device_cfg == nullptr) {
        ESP_LOGE(TAG, "%s: Device configuration is required ...aborting", __func__);
        set_restart(0);
        return;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IOT_EVENT, ESP_EVENT_ANY_ID, &on_event,
                                                        this, nullptr));

    _iot_wifi->start();

    if (!_iot_wifi->configured()) {
        ESP_LOGI(TAG, "%s: WiFi is not configured yet.", __func__);
        _app_state = IotAppState::CONFIGURING;

        _iot_provision = new IotProvision();

        _iot_provision->start();
    } else {
        iot_zero_mem(&_device_data, sizeof(iot_device_data_t));

        auto storage = IotFactory::create_scoped<IotStorage>(IOT_NVS_DEFAULT_PART_NAME, IOT_NVS_DEFAULT_NAMESPACE);

        ret = storage->read(IOT_NVS_DEVICE_DATA_KEY, &_device_data, sizeof(iot_device_data_t));

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "%s: Failed to load application data [reason: %s]", __func__, esp_err_to_name(ret));

            strncpy(_device_data.name, strcat(iot_char_s(("hover.")), _iot_wifi->get_mac()), IOT_MAX_ANY_NAME_LEN);
            strcpy(_device_data.uuid, _iot_wifi->get_mac());
        }

        init(config);
    }

    xTaskCreatePinnedToCore(&task, "iot_app_task", 6096, this, 4, nullptr, 1);

    ESP_LOGI(TAG, "%s: Component started successfully", __func__);
}

/**
 * Initializes the rest of the core components.
 *
 * @param config The application configuration.
 */
void IotApplication::init(iot_app_cfg_t config)
{
    auto server = &IotFactory::create_component<IotServer>();
    server->start();

    char *mac_address = _iot_wifi->get_mac();

    server->register_route("reboot", HTTP_GET, [](httpd_req_t *req) -> esp_err_t {
        iot_should_reboot_event_t  reboot;
        iot_event_queue_t event = {.id = IOT_APP_SHOULD_REBOOT_EVENT, .data = &reboot};
        send_to_queue(event);
        httpd_resp_sendstr(req, "Device will reboot");
        return ESP_OK;
    });

    _iot_wifi->init_mdns(_device_data.name);

    server->set_auth(_device_data.uuid);

    _components.push_back(server);

    iot_device_meta_t meta = iot_device_meta_t(mac_address, config.model, _app_desc->version,
                                               _app_desc->date);

    iot_device_cfg_t *device = config.device_cfg;

    device->device_info->device_name = _device_data.name;
    device->device_info->metadata = meta;
    device->device_info->uuid = _device_data.uuid;

    for (const auto &service: device->device_info->services) {
        if (service.name == IOT_OTA_SERVICE) {
            if (service.enabled) {
                _iot_ota = new IotOta();
                _iot_ota->init(const_cast<esp_app_desc_t *>(_app_desc));
            }
        }
    }

    _iot_device->init(std::move(config.device_cfg));
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

    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.google.com");
    esp_sntp_setservername(1, "pool.ntp.com");
    sntp_set_time_sync_notification_cb(on_sntp_update);
    sntp_set_sync_interval(_clock_sync_time);

    esp_sntp_init();

    ESP_LOGI(TAG, "%s: Done initializing sntp", __func__);
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
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_INFO);
    esp_log_level_set("mdns", ESP_LOG_INFO);
    esp_log_level_set("event:", ESP_LOG_INFO);
    esp_log_level_set("Iot", ESP_LOG_VERBOSE);
#endif
}

/**
 * Stops the component
 */
void IotApplication::stop()
{
    esp_event_handler_unregister(IOT_EVENT, ESP_EVENT_ANY_ID, &on_event);

    if (_iot_provision != nullptr) {
        delete _iot_provision;
    }

    delete _iot_device;

    if (_iot_ota != nullptr) {
        delete _iot_ota;
    }

    delete _iot_status;

    vQueueDelete(_queue_handle);
    _queue_handle = nullptr;

    vSemaphoreDelete(_task_lock);
}

/**
 * Handles network connected related functionalities.
 */
void IotApplication::on_connected()
{
    _iot_status->set_mode(IotLedMode::IOT_LED_STATIC);

    if (_first_connection) {
        init_sntp(_timezone);

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
        _iot_mqtt = &IotFactory::create_component<IotMqtt>();

        _iot_mqtt->start(std::string(_device_data.name) + "_" + std::string(_iot_wifi->get_mac()));
#endif
        _first_connection = false;
    }

    if (_app_state != IotAppState::RESTARTING) {
        _app_state = IotAppState::CONNECTED;

#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
        if(_iot_mqtt != nullptr && !_iot_mqtt->connected())
            _iot_mqtt->reconnect();
#endif
    }
}

/**
 * Sets the device to reboot.
 *
 * @param[in] delay The delay in milliseconds before the device restarts.
 */
void IotApplication::set_restart(uint64_t delay)
{
    std::lock_guard<std::mutex> lock(_reboot_mutex);

    ESP_LOGI(TAG, "%s: Request to reboot in [time: %llu]", __func__, delay);

    if (_restart_delay == 0) {
        restart();
    }

    if (_app_state == IotAppState::RESTARTING)
        ESP_LOGW(TAG, "%s: The application is in a reboot state already.", __func__);
    else {
        _restart_delay = delay;
        _app_state = IotAppState::RESTARTING;
    }
}

/**
 * Restarts the device.
 */
void IotApplication::restart(void)
{
    ESP_LOGI(TAG, "%s: Rebooting device............", __func__);

    vTaskDelay(pdMS_TO_TICKS(150));

    for (const auto &component: _components) {
        component->stop();
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    if (_restart_delay != 0 || _restart_delay > 400) {
        _restart_delay -= 300;
    }

    stop();
    vTaskDelay(pdMS_TO_TICKS(_restart_delay));
    esp_restart();
}

/**
 * Locks the task by acquiring the task and starts the timeout timer.
 *
 * @note The max timeout is a minute and 30 seconds. This function can block.
 */
void IotApplication::lock_task(void)
{
    if (_app_state == IotAppState::LOCKED) {
        ESP_LOGW(TAG, "%s: The application task is already locked", __func__);
        return;
    }

    _app_state = IotAppState::LOCKED;

    if (xSemaphoreTake(_task_lock, portMAX_DELAY) == pdTRUE) {
        if (xTimerStart(_lock_timeout, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "%s: Failed to start the timer", __func__);
            xSemaphoreGive(_task_lock);
        }
    } else {
        ESP_LOGE(TAG, "%s: Failed to acquire the task lock", __func__);
    }
}

/**
 * Unlocks task releasing the lock and stops the timeout timer.
 *
 * @note This function can block.
 */
void IotApplication::unlock_task(void)
{
    if (xSemaphoreGive(_task_lock) != pdTRUE) {
        ESP_LOGE(TAG, "%s: Failed to releasing the task lock", __func__);
    }

    if (xTimerStop(_lock_timeout, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "%s: Failed to stop the timer", __func__);
    }

    _app_state = IotAppState::RUNNING;
}

/**
 * Processes the application event queue.
 *
 * @param event The event to process.
 */
void IotApplication::process_event(iot_event_queue_t event)
{
    ESP_LOGI(TAG, "%s: Processing event [id: %u]", __func__, event.id);

    switch (event.id) {
        case IOT_APP_PROV_STARTED_EVENT:
            _iot_status->set_mode(IotLedMode::IOT_LED_FAST_BLINK);
            break;
        case IOT_APP_PROV_SUCCESS_EVENT:
            _app_state = IotAppState::CONFIGURED;
            _iot_status->set_mode(IotLedMode::IOT_LED_STATIC);
            set_restart(iot_convert_time_to_ms(IOT_REBOOT_SAFE_TIME));
            break;
        case IOT_APP_PROV_FAIL_EVENT:
            set_restart(0);
            break;
        case IOT_APP_WIFI_CONNECTED_EVENT:
            if (_app_state == IotAppState::CONFIGURING)
                return;
            on_connected();
            break;
        case IOT_APP_WIFI_CONNECTION_FAIL_EVENT:
            if (_app_state == IotAppState::CONFIGURING) {
                ESP_LOGI(TAG, "%s: Failed to connect, restarting device....", __func__);
                set_restart(0);
            }
            break;
        case IOT_APP_WIFI_RECONNECTING_EVENT:
            _app_state = IotAppState::CONNECTING;
            break;
        case IOT_APP_WIFI_RECONNECTION_FAIL_EVENT:
            set_restart(0);
            break;
        case IOT_APP_WIFI_DISCONNECTED_EVENT:
            _iot_status->set_mode(IotLedMode::IOT_LED_SLOW_BLINK);
            _app_state = IotAppState::CONNECTING;
            break;
        case IOT_APP_SHOULD_REBOOT_EVENT: {
            iot_should_reboot_event_t *req = static_cast<iot_should_reboot_event_t *>(event.data);

            uint64_t delay = req != nullptr ? req->delay : iot_convert_time_to_ms(IOT_REBOOT_SAFE_TIME);
            set_restart(delay);
            break;
        }
        case IOT_APP_LOCK_TASK_EVENT:
            lock_task();
            break;
#ifdef CONFIG_IOT_HOVER_MQTT_ENABLED
        case IOT_APP_MQTT_CONNECTED_EVENT:
            if (!_iot_device->subscribed_to_mqtt())
                _iot_device->subscribe_to_mqtt();
            break;
        case IOT_APP_MQTT_CONNECTION_FAIL_EVENT:
            // TODO: Should create an mqtt task perhaps ?
            break;
        case IOT_APP_MQTT_DISCONNECTED_EVENT:
            // TODO: Should create an mqtt task perhaps ?
            if(_iot_mqtt != nullptr && !_iot_mqtt->connected())
                _iot_mqtt->reconnect();
            break;
#endif
        default:
            ESP_LOGW(TAG, "Received unknown event [id: %u]", event.id);
            break;
    }
}

/**
 * Sends a message to the queue.
 *
 * @param[in] msg The message to send.
 * @return pdTrue on success, otherwise pdFALSE.
 */
BaseType_t IotApplication::send_to_queue(iot_event_queue_t msg)
{
    return xQueueSend(_queue_handle, &msg, portMAX_DELAY);
}

/**
 * Callback handler for time synchronization notifications, Sets the device's time to.
 *
 * @param[in] tv A pointer to the time received from the server.
 */
void IotApplication::on_sntp_update(timeval *tv)
{
    ESP_LOGI(TAG, "%s: Current [time: %s]", __func__, iot_now_str().data());
    settimeofday(tv, nullptr);
    sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}

/**
 * Handles application related events.
 *
 * @param[in] args A pointer to the user data.
 * @param[in] base The event base for the handler.
 * @param[in] id The id of the received event.
 * @param[in] data A pointer to the event data.
 */
void IotApplication::on_event([[maybe_unused]] void *args, esp_event_base_t base, int32_t id, void *data)
{
    if (base == IOT_EVENT)  {
        iot_event_queue_t message = {.id = static_cast<iot_app_event_e>(id), .data = data};

        if (id == IOT_APP_UNLOCK_TASK_EVENT) {
            unlock_task();
            return;
        }
        send_to_queue(message);
    }
}

/**
 * Task for the IotApplication component.
 *
 * @param[in] param A pointer to the task parameter (this).
 */
[[noreturn]] IRAM_ATTR void IotApplication::task(void *param)
{
    size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "%s: Task started running, current total heap [size: %u]", __func__, heap_size);

    auto *self = static_cast<IotApplication *>(param);

    if (self == nullptr)
        esp_system_abort("Pointer to iot app is null, Did you forget to pass it as a param to the task ?");

    if (heap_caps_check_integrity_all(true) == false)
    {
        esp_system_abort("Heap FAILED checks!");
    }

    xSemaphoreGive(_task_lock);

    static uint32_t last_check = 0;
    iot_event_queue_t event;

    while (true) {
        if (xSemaphoreTake(_task_lock, portMAX_DELAY) == pdTRUE) {
            xSemaphoreGive(_task_lock);

            if (_app_state == IotAppState::RESTARTING) {
                self->restart();
            }

            if (xQueueReceive(_queue_handle, &event, pdMS_TO_TICKS(1000)) == pdTRUE)
                self->process_event(event);

            if (iot_millis() - last_check > 10000) {
                last_check = iot_millis();
                size_t heap_free_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
                size_t heap_usage = heap_size - heap_free_size;

                ESP_LOGI(TAG, "%s: Task heap [usage: %u bytes]", __func__, heap_usage);
                ESP_LOGI(TAG, "%s: Task high stack [water mark: %d bytes]", __func__,
                         uxTaskGetStackHighWaterMark(NULL));
            }
        }
    }
}

/**
 *  Callback function for the lock timeout timer.
 *
 * @param xTimer The handle to the timer that triggered this callback.
 */
void IotApplication::lock_timeout([[maybe_unused]] TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "%s: Exceeded max time or lock or you forgot to free the lock ?", __func__);
    unlock_task();
}

IotApplication IotApp;