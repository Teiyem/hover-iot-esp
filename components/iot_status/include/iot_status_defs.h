#pragma once

/**
 * An enum of the different led modes.
 */
enum class IotLedMode {
    IOT_LED_NONE,		    /**< Indicates that the LED will be off. */
    IOT_LED_STATIC,		    /**< Indicates that the LED will not blink. */
    IOT_LED_SLOW_BLINK,     /**< Indicates that the LED will blink slowly. */
    IOT_LED_FAST_BLINK	    /**< Indicates that the LED will be blinking rapidly. */
};