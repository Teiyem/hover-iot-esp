#pragma once

#include "driver/gpio.h"

#define IOT_ON 0x1      /**<  Input/Output on state. */
#define IOT_ON_IVT 0x0  /**<  Input/Output inverted on state. */
#define IOT_OFF 0x0     /**<  Input/Output off state. */
#define IOT_OFF_IV 0x1  /**<  Input/Output inverted off state. */

/**
 * Enum of the input mode of gpio pins.
 */
enum class IotGpioMode {
    Input = GPIO_MODE_INPUT,                    /**< Indicates that the GPIO pin is configured as an input.  */
    Output = GPIO_MODE_OUTPUT,                  /**< Indicates that the GPIO pin is configured as an output.  */
    MomentaryOutput = GPIO_MODE_OUTPUT,         /**< Indicates that the GPIO pin is configured as a momentary output.  */
    InputOutput = GPIO_MODE_INPUT_OUTPUT,       /**< Indicates that the GPIO pin is configured as both input and output. */
    InputOutputOD = GPIO_MODE_INPUT_OUTPUT_OD   /**< Indicates that the GPIO pin is configured as input/output with open-drain. */
};