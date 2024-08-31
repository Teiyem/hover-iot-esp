#include "iot_gpio.h"

/**
 * Sets the gpio mode of the pin.
 *
 * @param mode The mode to set the pin to.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioBase::mode(IotGpioMode mode)
{
    esp_err_t ret = gpio_set_direction(_pin, static_cast<gpio_mode_t>(mode));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to set pin mode, [reason %s]", __func__, esp_err_to_name(ret));
    }

    return ret;
}

/**
 * Gets the state's string representation.
 *
 * @return The string representation of the state.
 */
std::string IotGpioBase::state_str(void)
{
    return state() == IOT_ON ? "On" : "Off";
}

/**
 * Gets the input state.
 *
 * @return The input pin state.
 */
bool IotGpioInput::state(void)
{
    _state = gpio_get_level(_pin);
    return _state;
}

/**
 * Enables the interrupt for the input.
 *
 * @param intr_type  The type of interrupt to enable (e.g., rising edge, falling edge).
 * @param callback   The callback function to invoked when the interrupt occurs.
 * @param debounce_ms The debounce time in milliseconds to prevent multiple triggers. Default is 35 ms
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioInput::enable_interrupt(gpio_int_type_t intr_type, IotGpioCallback callback, int debounce_ms)
{
    esp_err_t ret = intr_type == GPIO_INTR_DISABLE ? ESP_ERR_INVALID_ARG : ESP_OK;

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "%s: Cannot use interrupt type GPIO_INTR_DISABLE to enable interrupt, Did you intend to disable interrupt ?", __func__);
        return ret;
    }

    ret = gpio_set_intr_type(_pin, intr_type);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to enable interrupt, [reason %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_isr_handler_add(_pin, [](void* arg) {
        auto self = static_cast<IotGpioInput*>(arg);
        auto now = iot_millis();

        if (now - self->_last_interrupt > self->_debounce_ms) {
            self->_last_interrupt = now;
            if (self->_callback) {
                self->_callback();
            }
        }
    }, this);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to add interrupt handler, [reason %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    _callback = callback;
    _debounce_ms = debounce_ms;

    return ret;
}

/**
 * Disables the interrupt for the input.
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioInput::disable_interrupt(void)
{
    esp_err_t ret = gpio_isr_handler_remove(_pin);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s: Failed to remove interrupt handler, [reason %s]", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = gpio_set_intr_type(_pin, GPIO_INTR_DISABLE);

    if (ret != ESP_OK)
        ESP_LOGE(TAG, "%s: Failed to disable interrupt, [reason %s]", __func__, esp_err_to_name(ret));

    return ret;
}

/**
 * Initialises a new instance of the IotGpioOutput class.
 * @param pin The gpio pin.
 * @param inverted  Whether the output logic is inverted. Default is false.
 * @param momentary Whether the output is momentary. Default is false.
 * @param pullup  Whether the pull-up resistor should be enabled. Default is false.
 * @param pulldown  Whether the pull-down resistor should be enabled. Default is false.
*/
IotGpioOutput::IotGpioOutput(const gpio_num_t pin, const bool inverted, const bool momentary, bool pullup,
                             bool pulldown) : IotGpioBase(pin, momentary ? IotGpioMode::MomentaryOutput :
                             IotGpioMode::Output, inverted, pullup, pulldown), _momentary{momentary}

{
    if (momentary) {
        _timer = xTimerCreate("iot_output_timer", pdMS_TO_TICKS(_duration_ms),
                              pdFALSE, this, &timer_cb);
    }
}

/**
 * Gets the output state.
 *
 * @return The output state.
 */
bool IotGpioOutput::state(void)
{
    return _state;
}

/**
 * Sets the output state.
 *
 * @param state The pin state to set.
 */
esp_err_t IotGpioOutput::set(bool state)
{
    esp_err_t ret = gpio_set_level(_pin, _inverted ? !state : state);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "%s: Failed to set pin state, [reason %s]", __func__, esp_err_to_name(ret));
    } else
        _state = state;
    if (_momentary && _timer != nullptr)
        xTimerStart(_timer, 0);

    return ret;
}

/**
 * Toggles the output pin.
 *
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioOutput::toggle()
{
    return set(!state());
}

/**
 * The callback for the momentary timer callback
 * @param xTimer The timer handle.
 */
void IotGpioOutput::timer_cb(TimerHandle_t xTimer)
{
    IotGpioOutput* self = static_cast<IotGpioOutput*>(pvTimerGetTimerID(xTimer));

    self->set(IOT_OFF);
}

/**
 * Initialises a new instance of the IotGpioOutputGroup class.
 * @param pins The list of gpio pin.
 * @param mode The gpio mode.
 * @param inverted Whether the toggle logic is inverted.
 * @param pullup  Whether the pull-up resistor should be enabled.
 * @param pulldown  Whether the pull-down resistor should be enabled.
 */
IotGpioOutputGroup::IotGpioOutputGroup(std::initializer_list<gpio_num_t> pins, bool inverted, bool pullup, bool pulldown)
{
    for (auto pin : pins) {
        _outputs.emplace_back(pin, inverted);
    }
}

/**
 * Sets the state of all the outputs.
 *
 * @param state The state to set the outputs to.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioOutputGroup::set(bool state)
{   esp_err_t ret = ESP_OK;

    for (auto& output : _outputs) {
        ret = output.set(state);
        if (ret != ESP_OK)
            break;
    }

    return  ret;
}

/**
 * Toggles the state of all outputs.
 */
esp_err_t IotGpioOutputGroup::toggle()
{
    esp_err_t ret = ESP_FAIL;
    for (auto& output : _outputs) {
        ret = output.toggle();
        if (ret != ESP_OK)
            break;
    }
    return  ret;
}

/**
 * Sets the state of one a single output,
 *
 * @param index The index of the output to set the state of.
 * @param state The state to set the output to.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioOutputGroup::set_one(size_t index, bool state)
{
    esp_err_t ret = ESP_FAIL;

    if (index < _outputs.size()) {
      ret = _outputs[index].set(state);
    }

    return  ret;
}

/**
 * Toggles the state of a single output.
 * @param index  The index of the output to toggle the state of.
 * @return ESP_OK on success, otherwise an error code.
 */
esp_err_t IotGpioOutputGroup::toggle_one(size_t index)
{
    esp_err_t ret = ESP_FAIL;
    if (index < _outputs.size()) {
        ret = _outputs[index].toggle();
    }
    return ret;
}

/**
 * Gets the state of a single output.
 *
 * @param index The index to get the state of.
 * @return  The state of the output.
 */
bool IotGpioOutputGroup::state(size_t index)
{
    if (index < _outputs.size()) {
        return _outputs[index].state();
    }
    return false;
}

/**
 * Gets the state of all outputs.
 *
 * @return The states of all the outputs.
 */
std::vector<bool> IotGpioOutputGroup::states()
{
    std::vector<bool> states;
    for (auto& output : _outputs) {
        states.push_back(output.state());
    }
    return states;
}
