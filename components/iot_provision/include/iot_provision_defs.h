#pragma once

#define IOT_PROV_MSG_START 41         /**< The start of the iot application messages. Reserved 41 to 43. */

/**
 * An enum for provision messages.
 */
typedef enum iot_prov_message
{
    IOT_PROV_MSG_STARTED = IOT_PROV_MSG_START,  /**< Indicates that provisioning is started. */
    IOT_PROV_MSG_SUCCESS,                       /**< Indicates that provisioning was success. */
    IOT_PROV_MSG_FINISHED,                      /**< Indicates that provisioning is finished. */
    IOT_PROV_MSG_FAIL,                          /**< Indicates that provisioning failed. Most likely invalid credentials. */
} iot_prov_message_e;