#pragma once

#define IOT_PROV_QRCODE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"
#define IOT_PROV_QR_VERSION         "v1"

#define IOT_PROV_MSG_START 41 /**< The start of the iot application messages. Reserved 41 to 43. */

/**
 * An enum of setup messages types.
 */
typedef enum iot_prov_message
{
    IOT_PROV_MSG_STARTED = IOT_PROV_MSG_START, /**< The provisioning was success. */
    IOT_PROV_MSG_SUCCESS,                      /**< The provisioning was success. */
    IOT_PROV_MSG_FINISHED,                     /**< The provisioning is finished. */
    IOT_PROV_MSG_FAIL,                         /**< The provisioning failed. Most likely invalid credentials. */
} iot_prov_message_e;