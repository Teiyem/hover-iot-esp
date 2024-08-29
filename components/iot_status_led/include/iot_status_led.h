#pragma once

#include "iot_status_led_defs.h"
#include "iot_common.h"
#include "iot_component.h"
#include "driver/gpio.h"

/**
 * A class that handles the LED status related tasks.
 */
class IotStatusLed final : public IotComponent
{
public:
    /**
     * Initialises a new instance of the IotStatusLed class.
     * @param pin The status LED's pin number.
     * @param inverted Whether the toggle logic is inverted.
     */
    constexpr IotStatusLed(const gpio_num_t pin, const bool inverted) :
            _pin{pin},
            _inverted{inverted},
            _cfg{gpio_config_t{
                    .pin_bit_mask   = static_cast<uint32_t>(1) << pin,
                    .mode           = GPIO_MODE_OUTPUT,
                    .pull_up_en     = GPIO_PULLUP_DISABLE,
                    .pull_down_en   = GPIO_PULLDOWN_ENABLE,
                    .intr_type      = GPIO_INTR_DISABLE
            },
            }
    { }

    ~IotStatusLed(void);
    esp_err_t start(void) override;
    void stop(void) override;
    esp_err_t toggle(bool state);
    void set_mode(iot_led_mode_e mode);
    bool state(void) const;

private:
    static constexpr const char *TAG = "IotStatusLed";  /** A constant used to identify the source of the log message of this class. */
    const gpio_num_t _pin;                              /** The pin number that the status LED is connected to. */
    const bool _inverted;                               /** Whether the toggle logic is inverted. */
    bool _state = false;                                /** The state of the led. */
    const gpio_config_t _cfg;                           /** The Led's GPIO pin configuration. */
    static iot_led_mode_e _mode;
    uint32_t _last_toggle = 0;                          /** The last time the LED was toggled. */
    const uint32_t _slow_blink = 1000;                  /** Slow blink rate of the LED. */
    const uint32_t _fast_blink = 300;                   /** Fast blink rate of the LED. */
    SemaphoreHandle_t _toggle_mutex = nullptr;           /** The semaphore used to safe guard calls to toggles. */

    static TaskHandle_t _task_handle;

    void handle(void);
    [[noreturn]] static void task(void *param);
};