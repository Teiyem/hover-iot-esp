#pragma once

#include "iot_common.h"

#define IOT_WIFI_MSG_START 21 /**< The start of the iot wifi messages. Reserved 21 to 40. */

/**
 * An enum for Wi-Fi messages.
 */
typedef enum iot_wifi_message
{
    IOT_WIFI_MSG_STARTED = IOT_WIFI_MSG_START, /**< The wifi has started. */
    IOT_WIFI_MSG_CONNECTED,                    /**< The wifi is connected. */
    IOT_WIFI_MSG_DISCONNECTED,                 /**< The wifi is disconnected. */
    IOT_WIFI_MSG_CONNECT_FAILED,               /**< The wifi connection failed. */
    IOT_WIFI_MSG_RECONNECTING,                 /**< The wifi is reconnecting. */
    IOT_WIFI_MSG_RECONNECTING_FAIL = 26,       /**< The wifi reconnection failed. */
} iot_wifi_message_e;

/**
 * A struct for WiFi access point settings.
 */
typedef struct iot_wifi_ap_settings
{
    char ssid[32];     /**< The access point's SSID . */
    char password[64]; /**< The access point's password. */
    char ip[16];       /**< The access point's IP address. */
    char gateway[16];  /**< The access point's gateway address. */
    char netmask[16];  /**< The access point's netmask. */
} iot_wifi_ap_settings_t;