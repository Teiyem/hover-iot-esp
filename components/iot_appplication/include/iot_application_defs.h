 #pragma once

#include "iot_defs.h"

/**
 * A struct for the application configuration.
 */
typedef struct iot_app_cfg
{
    iot_wifi_op_mode_e op_mode;      /**< The wifi operating mode.*/
    iot_device_cfg_t *device_cfg;    /**< The device configuration. */
    std::string model;               /**< The device model. */
} iot_app_cfg_t;

/**
 * An enum of the different application states.
 */
enum class iot_app_state_e
{
    INITIAL,       /**<  The initial state when the application starts up for the first time. */
    RUNNING,       /**< The state where the device is running as normal. */
    CONFIGURING,   /**< The state where the device is being configured. */
    CONFIGURED,    /**< The state where the device is configured. */
    CONNECTING,    /**< The state where the device is attempting to connect to the network. */
    CONNECTED,     /**< The state where the device is connected to the network. */
    UPDATING,      /**< The state where the device is updating. */
    ERROR,         /**< The state where the device has to reboot. */
    REBOOTING      /**< The state where the device is rebooting. */
};