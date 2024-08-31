#pragma once

#include <functional>
#include "iot_common.h"
#include "iot_gpio_defs.h"

using IotGpioCallback = std::function<void(void)>;

/**
 * A base class for gpio pins
 */
class IotGpioBase {
public:
    /**
     * Initialises a new instance of the IotGpioBase class.
     * @param pin The gpio pin.
     * @param mode The gpio mode.
     * @param inverted Whether the gpio logic is inverted. Default is false
     * @param pullup  Whether the pull-up resistor should be enabled. Default is false
     * @param pulldown  Whether the pull-down resistor should be enabled. Default is false
     */
    constexpr IotGpioBase(const gpio_num_t pin, IotGpioMode mode, const bool inverted = false, bool pullup = false, bool pulldown = false)  : _pin{pin}, _inverted{inverted}
    {
        gpio_config_t io_conf = {
                .pin_bit_mask   = static_cast<uint32_t>(1) << pin,
                .mode           = static_cast<gpio_mode_t>(mode),
                .pull_up_en     = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
                .pull_down_en   = pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
                .intr_type      = GPIO_INTR_DISABLE
        };

        gpio_config(&io_conf);
    }
    virtual ~IotGpioBase() = default;

    esp_err_t mode(IotGpioMode mode);
    std::string state_str(void);

protected:
    static constexpr const char *TAG = "IotGpio";   /** A constant used to identify the source of the log message of this class. */
    const gpio_num_t _pin;                          /** The pin number of the gpio. */
    bool _inverted = false;                         /** Whether the toggle logic is inverted. */
    virtual bool state(void) = 0;
};

/**
 * A class that handles controlling GPIO input pins.
 */
class IotGpioInput : public IotGpioBase {
public:
    /**
       * Initialises a new instance of the IotGpioInput class.
       * @param pin The gpio pin.
       * @param pullup  Whether the pull-up resistor should be enabled. Default is false
       * @param pulldown  Whether the pull-down resistor should be enabled. Default is false
       */
    explicit IotGpioInput(const gpio_num_t pin, bool pullup = false, bool pulldown = false)
    : IotGpioBase(pin, IotGpioMode::Input,false, pullup, pulldown) {}

    bool state(void) override;
    esp_err_t enable_interrupt(gpio_int_type_t intr_type, IotGpioCallback callback, int debounce_ms = 50);
    esp_err_t disable_interrupt(void);

private:
    bool _state;                                        /** The state of the input. */
    IotGpioCallback _callback;                          /** The interrupt callback handler. */
    int _debounce_ms = 50;                              /** The debounce milliseconds. */
    uint32_t _last_interrupt = 0;                       /** The last time the interrupt occurred in milliseconds. */
};

/**
 * A class that handles controlling GPIO output pins.
 */
class IotGpioOutput : public IotGpioBase {
public:
    explicit IotGpioOutput(const gpio_num_t pin, const bool inverted = false, const bool momentary = false, bool pullup = false, bool pulldown = false);

    bool state(void) override;
    esp_err_t set(bool state);
    esp_err_t toggle();

private:
    bool _state;                                       /** The state of the output. */
    const bool _momentary;                             /** Whether the output is momentary. */
    uint32_t _duration_ms;                             /** The duration to keep the pin high. */
    TimerHandle_t _timer;
    static void timer_cb(TimerHandle_t xTimer);
};

/**
 * A class that handles controlling multiple GPIO output pins as a group.
 */
class IotGpioOutputGroup  {
public:
    IotGpioOutputGroup(std::initializer_list<gpio_num_t> pins, bool inverted, bool pullup, bool pulldown);

    esp_err_t set(bool state);
    esp_err_t toggle();
    esp_err_t set_one(size_t index, bool state);
    esp_err_t toggle_one(size_t index);

    bool state(size_t index);
    std::vector<bool> states();

private:
    std::vector<IotGpioOutput> _outputs;
};