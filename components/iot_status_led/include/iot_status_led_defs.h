#pragma once

/**
 * An enum of the different led modes.
 */
typedef enum iot_led_mode {
    IOT_STATIC,		    /* Indicates that the LED will not blink. */
    IOT_SLOW_BLINK,     /* Indicates that the LED will blink slowly. */
    IOT_FAST_BLINK	    /* Indicates that the LED will be blinking rapidly. */
} iot_led_mode_e;