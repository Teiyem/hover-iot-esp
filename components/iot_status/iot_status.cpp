#include "iot_status.h"

/** The led's current mode. */
IotLedMode IotStatus::_mode{IotLedMode::IOT_LED_STATIC};

/** The task handle. */
TaskHandle_t IotStatus::_task_handle{nullptr};

/**
 * Starts the status led component.
 */
esp_err_t IotStatus::start(void)
{
    if (_started) {
        ESP_LOGW(TAG, "%s: Component is already started", __func__);
        return ESP_OK;
    }

    _toggle_mutex = xSemaphoreCreateMutex();

    esp_err_t result = set(_inverted);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to toggle led [reason: %s]", __func__, esp_err_to_name(result));
    }

    xTaskCreate(task, "iot_status", 2048, this, 1, &_task_handle);

    _started = true;

    ESP_LOGI(TAG, "%s: Starting component", __func__);

    return ESP_OK;
}

/**
 * Destroys the IotStatus class.
 */
IotStatus::~IotStatus(void)
{
    if (_started){
        stop();
    }
}

/**
 * Stops the components.
 */
void IotStatus::stop(void)
{
    vTaskDelete(_task_handle);
    _task_handle = nullptr;
    if (_toggle_mutex)
        vSemaphoreDelete(_toggle_mutex);
    _toggle_mutex = nullptr;
}

/**
 * Set the mode of the LED.
 *
 * @param mode The mode to use.
 */
void IotStatus::set_mode(IotLedMode mode)
{
    ESP_LOGI(TAG, "%s: Setting the led mode [to: %d]", __func__, static_cast<int>(mode));
    _mode = mode;
}

/**
 * Task for the IotStatus component.
 *
 * @param[in] param A pointer to the task parameter (this).
 */
[[noreturn]] IRAM_ATTR void IotStatus::task(void *param)
{
    size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "%s: Task started running, current total heap [size: %u bytes]", __func__, heap_size);

    auto *self = static_cast<IotStatus *>(param);

    iot_not_null(self);

    while (true)
    {
        if (_mode != IotLedMode::IOT_LED_STATIC) {
            const auto timeout = _mode == IotLedMode::IOT_LED_SLOW_BLINK ? self->_slow_blink : self->_fast_blink;

            if (iot_millis() - self->_last_toggle > timeout) {
                const auto result = self->toggle();

                if (result == ESP_OK)
                    self->_last_toggle = iot_millis();
            }
        } else {
            if (self->state() != IOT_ON_IVT)
                self->toggle();
        }

        vTaskDelay(600 / portTICK_PERIOD_MS);
    }
}

