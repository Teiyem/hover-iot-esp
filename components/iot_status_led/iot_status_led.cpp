#include "iot_status_led.h"

/** The led's current mode. */
iot_led_mode_e IotStatusLed::_mode{IOT_LED_STATIC};

/** The task handle. */
TaskHandle_t IotStatusLed::_task_handle{nullptr};

/**
 * Starts the status led component.
 */
esp_err_t IotStatusLed::start(void)
{
    if (_started) {
        ESP_LOGW(TAG, "%s: Component is already started", __func__);
        return ESP_OK;
    }

    _toggle_mutex = xSemaphoreCreateMutex();

    auto result = gpio_config(&_cfg);

    if (result == ESP_OK)
    {
        result = toggle(_inverted);

        if (result != ESP_OK) {
            ESP_LOGE(TAG, "%s: Failed to toggle led [reason: %s]", __func__, esp_err_to_name(result));
        }
    }

    xTaskCreate(task, "iot_status_led", 4000, this, 1, &_task_handle);

    _started = true;

    ESP_LOGI(TAG, "%s: Starting component", __func__);

    return ESP_OK;
}

/**
 * Destroys the IotStatusLed class.
 */
IotStatusLed::~IotStatusLed(void)
{
    if (_started){
        stop();
    }
}

/**
 * Stops the components.
 */
void IotStatusLed::stop(void)
{
    vTaskDelete(_task_handle);
    _task_handle = nullptr;
    if (_toggle_mutex)
        vSemaphoreDelete(_toggle_mutex);
    _toggle_mutex = nullptr;
}

/**
 * Toggles the state of the LED.
 *
 * @param state The state to toggle the LED to.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotStatusLed::toggle(bool state)
{
    BaseType_t ret = xSemaphoreTake(_toggle_mutex, 10 / portTICK_PERIOD_MS);

    if (ret != pdTRUE)
        return ESP_FAIL;

    _state = state;
    ESP_LOGI(TAG, "%s: Toggling the led [to: %s]", __func__, _state? "on" : "off");

    xSemaphoreGive(_toggle_mutex);

    return gpio_set_level(_pin, _inverted ? !state : state);
}

/**
 * Set the mode of the LED.
 *
 * @param mode The mode to use.
 */
void IotStatusLed::set_mode(const iot_led_mode_e mode)
{
    ESP_LOGI(TAG, "%s: Setting the led mode [to: %d]", __func__, mode);
    _mode = mode;
}

/**
 * Handles toggling the led on or off depending on the current mode.
 */
void IotStatusLed::handle(void)
{
    if (_mode == IOT_LED_STATIC) return;

    const auto timeout = _mode == IOT_LED_SLOW_BLINK ? _slow_blink : _fast_blink;

    if (iot_millis() - _last_toggle > timeout)
    {
        const auto result = toggle(!_state);

        if (result == ESP_OK)
            _last_toggle = iot_millis();
    }
}

/**
 * Task for the IotStatusLed component.
 *
 * @param[in] param A pointer to the task parameter (this).
 */
[[noreturn]] IRAM_ATTR void IotStatusLed::task(void *param)
{
    size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "%s: Task started running, current total heap [size: %u bytes]", __func__, heap_size);

    auto *self = static_cast<IotStatusLed *>(param);

    iot_not_null(self);

    while (true)
    {
        self->handle();
        vTaskDelay(600 / portTICK_PERIOD_MS);
    }
}

/**
 * Get the state of the led.
 *
 * @return The led state
 */
bool IotStatusLed::state(void) const
{
    return  _inverted ? !_state : _state;
}


