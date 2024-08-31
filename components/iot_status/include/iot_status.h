#pragma once

#include "iot_status_defs.h"
#include "iot_common.h"
#include "iot_component.h"
#include "iot_gpio.h"

/**
 * A class that handles the LED status related tasks.
 */
class IotStatus final : public IotGpioOutput, public IotComponent
{
public:
    /**
     * Initialises a new instance of the IotStatus class.
     * @param pin The status LED's pin number.
     * @param inverted Whether the toggle logic is inverted.
     */
    IotStatus(const gpio_num_t pin, const bool inverted) : IotGpioOutput(pin, inverted){ }

    ~IotStatus(void);
    esp_err_t start(void) override;
    void stop(void) override;
    void set_mode(IotLedMode mode);

private:
    static constexpr const char *TAG = "IotStatus";  /** A constant used to identify the source of the log message of this class. */
    static IotLedMode _mode;
    uint32_t _last_toggle = 0;                          /** The last time the LED was toggled. */
    const uint32_t _slow_blink = 1000;                  /** Slow blink rate of the LED. */
    const uint32_t _fast_blink = 300;                   /** Fast blink rate of the LED. */
    SemaphoreHandle_t _toggle_mutex = nullptr;          /** The semaphore used to safe guard calls to toggles. */

    static TaskHandle_t _task_handle;

    [[noreturn]] static void task(void *param);
};